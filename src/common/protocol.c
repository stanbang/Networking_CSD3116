/* Wire validation, clocks and locks. Author: <team>, Codex assistance. Date: 2026-11-03. */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <openssl/rand.h>
#include "common/protocol.h"

/* All-zero addressing is reserved so missing/zeroed joins fail closed. */
static bool nonce_valid(const uint8_t arr_nonce[16])
{
    uint8_t int_bits = 0;
    for (unsigned int int_i = 0; int_i < 16; ++int_i) int_bits |= arr_nonce[int_i];
    return int_bits != 0;
}

/* Pure structural checks; admission and replay history remain server-owned. */
bool join_valid(const Ast_Join *str_join)
{
    return str_join && str_join->int_version == AST_VERSION && str_join->int_seq &&
        nonce_valid(str_join->arr_nonce);
}

/* A rejection carries no usable slot/session; success must carry both. */
bool response_valid(const Ast_Response *str_response)
{
    if (!str_response || str_response->int_version != AST_VERSION || !str_response->int_seq ||
        str_response->int_status > 1 || !nonce_valid(str_response->arr_nonce)) return false;
    return str_response->int_status == 1 ?
        str_response->int_player_id == 0 && str_response->int_session == 0 :
        str_response->int_player_id >= 1 && str_response->int_player_id <= AST_PLAYERS && str_response->int_session;
}

/* Pure validation; no caller may index a player before this succeeds. */
bool input_valid(const Ast_Input *str_input)
{
    return str_input && str_input->int_version == AST_VERSION &&
        str_input->int_player_id >= 1 && str_input->int_player_id <= AST_PLAYERS &&
        str_input->int_session && str_input->int_seq && !(str_input->int_keys & ~AST_KEYS);
}

/* Leave has the same session and sequence constraints as input. */
bool leave_valid(const Ast_Leave *str_leave)
{
    return str_leave && str_leave->int_version == AST_VERSION &&
        str_leave->int_player_id >= 1 && str_leave->int_player_id <= AST_PLAYERS &&
        str_leave->int_session && str_leave->int_seq;
}

/* Length checks precede every buffer access, even for malicious local fixtures. */
static bool sequence_valid(uint32_t int_length, uint32_t int_maximum, uint32_t int_cap, const void *ptr_buffer)
{
    return int_length <= int_maximum && int_length <= int_cap && (!int_length || ptr_buffer);
}

/* Network coordinates are hundredths of pixels, not client-authoritative floats. */
static bool position_valid(int32_t int_x, int32_t int_y)
{
    return int_x >= 0 && int_x < AST_WIDTH * 100 && int_y >= 0 && int_y < AST_HEIGHT * 100;
}

/* Ships use keyed slot ids and bounded health/timers; no duplicate slot is rendered. */
static bool ships_valid(const Ast_Snapshot *str_snapshot, unsigned int *int_seen)
{
    for (uint32_t int_i = 0; int_i < str_snapshot->arr_ships._length; ++int_i) {
        const Ast_Ship *str_ship = &str_snapshot->arr_ships._buffer[int_i];
        if (str_ship->int_player_id < 1 || str_ship->int_player_id > AST_PLAYERS ||
            !position_valid(str_ship->int_x,str_ship->int_y) || str_ship->int_lives > AST_LIVES ||
            str_ship->int_respawn > AST_RESPAWN_TICKS || str_ship->int_invulnerable > AST_INVULNERABLE_TICKS ||
            (int64_t)str_ship->int_vx * str_ship->int_vx + (int64_t)str_ship->int_vy * str_ship->int_vy >
                (int64_t)(AST_SPEED_LIMIT * 100 + 2) * (int64_t)(AST_SPEED_LIMIT * 100 + 2)) return false;
        unsigned int int_bit = 1u << (str_ship->int_player_id - 1);
        if (*int_seen & int_bit) return false;
        *int_seen |= int_bit;
    }
    return true;
}

/* Entity ids and owner slots are checked independently of DDS wire-type bounds. */
static bool entities_valid(const Ast_Snapshot *str_snapshot, unsigned int int_seen)
{
    for (uint32_t int_i = 0; int_i < str_snapshot->arr_bullets._length; ++int_i) {
        const Ast_Bullet *str_bullet = &str_snapshot->arr_bullets._buffer[int_i];
        if (!str_bullet->int_id || str_bullet->int_owner < 1 || str_bullet->int_owner > AST_PLAYERS ||
            !(int_seen & (1u << (str_bullet->int_owner - 1))) || !position_valid(str_bullet->int_x,str_bullet->int_y)) return false;
        for (uint32_t int_j = 0; int_j < int_i; ++int_j)
            if (str_snapshot->arr_bullets._buffer[int_j].int_id == str_bullet->int_id) return false;
    }
    for (uint32_t int_i = 0; int_i < str_snapshot->arr_asteroids._length; ++int_i) {
        const Ast_Asteroid *str_asteroid = &str_snapshot->arr_asteroids._buffer[int_i];
        if (!str_asteroid->int_id || str_asteroid->int_size < 1 || str_asteroid->int_size > 3 ||
            !position_valid(str_asteroid->int_x,str_asteroid->int_y)) return false;
        for (uint32_t int_j = 0; int_j < int_i; ++int_j)
            if (str_snapshot->arr_asteroids._buffer[int_j].int_id == str_asteroid->int_id) return false;
    }
    return true;
}

/* Validate all sequence metadata before any entity field is inspected. */
bool snapshot_valid(const Ast_Snapshot *str_snapshot)
{
    if (!str_snapshot || str_snapshot->int_version != AST_VERSION || !str_snapshot->int_seq ||
        !str_snapshot->int_wave || str_snapshot->int_tick > (uint64_t)AST_HZ * AST_MAX_DURATION ||
        !sequence_valid(str_snapshot->arr_ships._length,str_snapshot->arr_ships._maximum,AST_PLAYERS,str_snapshot->arr_ships._buffer) ||
        !sequence_valid(str_snapshot->arr_bullets._length,str_snapshot->arr_bullets._maximum,AST_BULLETS,str_snapshot->arr_bullets._buffer) ||
        !sequence_valid(str_snapshot->arr_asteroids._length,str_snapshot->arr_asteroids._maximum,AST_ASTEROIDS,str_snapshot->arr_asteroids._buffer)) return false;
    unsigned int int_seen = 0;
    return ships_valid(str_snapshot,&int_seen) && entities_valid(str_snapshot,int_seen);
}

/* A receipt may be newer than main's sampled time while it waits for the lock. */
bool lease_expired(uint64_t int_now, uint64_t int_seen)
{
    return int_now >= int_seen && int_now - int_seen >= AST_LEASE_NS;
}

/* Monotonic time prevents clock changes from extending leases. */
uint64_t monotonic_ns(void)
{
    struct timespec str_time = {0};
    if (clock_gettime(CLOCK_MONOTONIC, &str_time) != 0) { perror("clock_gettime"); exit(1); }
    return (uint64_t)str_time.tv_sec * UINT64_C(1000000000) + (uint64_t)str_time.tv_nsec;
}

/* Callers hold no mutex while sleeping. Interrupted sleeps resume. */
void sleep_ns(uint64_t int_ns)
{
    struct timespec str_time = {(time_t)(int_ns / UINT64_C(1000000000)),
                               (long)(int_ns % UINT64_C(1000000000))};
    while (nanosleep(&str_time, &str_time) != 0) {
        if (errno != EINTR) { perror("nanosleep"); exit(1); }
    }
}

/* Protocol randomness must not come from the simulation PRNG. */
bool random_bytes(void *ptr_data, size_t int_size)
{
    return int_size <= INT_MAX && RAND_bytes(ptr_data, (int)int_size) == 1;
}

/* Lock failure cannot leave shared-state operations safely running. */
void mutex_lock(pthread_mutex_t *str_mutex)
{
    int int_result = pthread_mutex_lock(str_mutex);
    if (int_result) { fprintf(stderr, "mutex_lock: %s\n", strerror(int_result)); exit(1); }
}

/* Paired with mutex_lock, never spanning DDS I/O. */
void mutex_unlock(pthread_mutex_t *str_mutex)
{
    int int_result = pthread_mutex_unlock(str_mutex);
    if (int_result) { fprintf(stderr, "mutex_unlock: %s\n", strerror(int_result)); exit(1); }
}
