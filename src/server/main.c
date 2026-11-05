/**
 * @file
 * @brief Authoritative simulation loop, reconnect grace handling and server lifetime.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "common/account_paths.h"
#include "server/server.h"
static volatile sig_atomic_t b_signal = 0;

/* Signal handlers touch only sig_atomic_t state. */
static void stop_signal(int int_signal) { (void)int_signal; b_signal = 1; }

/* Provenance stays with the simulated ship even when its slot is reused. */
static void capture_score(Server *str_server, unsigned int int_slot)
{
    const ScoreOwner *str_owner = &str_server->arr_score_owner[int_slot];
    if (!str_owner->int_account_id) return;
    AccountScore str_score = {.int_account_id = str_owner->int_account_id,
        .int_score = str_server->str_sim.arr_ships[int_slot].int_score};
    if (!str_score.int_score) return;
    memcpy(str_score.arr_round_key, str_server->arr_boot_id, sizeof(str_server->arr_boot_id));
    for (unsigned int int_i = 0; int_i < 8; ++int_i) {
        str_score.arr_round_key[16 + int_i] = (uint8_t)(str_server->int_round >> (56 - 8 * int_i));
        str_score.arr_game_session[int_i] = (uint8_t)(str_owner->int_session >> (56 - 8 * int_i));
    }
    if (!accounts_submit_score(str_server->ptr_accounts, str_owner->int_identity, &str_score)) {
        atomic_store(&str_server->b_failed, true);
        atomic_store(&str_server->b_stop, true);
    }
}

static void capture_scores(Server *str_server)
{
    for (unsigned int int_i = 0; int_i < AST_PLAYERS; ++int_i) capture_score(str_server, int_i);
}

void server_release_slot(Server *str_server, unsigned int int_slot)
{
    Slot *str_slot = &str_server->arr_slots[int_slot];
    capture_score(str_server, int_slot);
    accounts_game_left(str_server->ptr_accounts, str_slot->int_identity, str_slot->arr_nonce);
    /* Keep the ship/account reservation until reauthentication or round end. */
    str_slot->b_disconnected = true;
    str_slot->int_keys = 0;
}

/* Explicit local acceptance fixture, never enabled by a network message.
 * Set up real collisions once; only sim_tick may increment evidence counters. */
static bool prepare_gameover_test(Server *str_server)
{
    if(!str_server->str_options.b_test_gameover||str_server->b_test_prepared||str_server->str_sim.int_tick<3*AST_HZ) return false;
    unsigned int int_active=0;
    for(unsigned int int_i=0;int_i<AST_PLAYERS;++int_i) int_active+=str_server->arr_slots[int_i].b_active;
    if(int_active<2) return false;
    Sim *str_sim=&str_server->str_sim;
    memset(str_sim->arr_bullets,0,sizeof(str_sim->arr_bullets));
    memset(str_sim->arr_asteroids,0,sizeof(str_sim->arr_asteroids));
    for(unsigned int int_i=0;int_i<AST_PLAYERS;++int_i) if(str_server->arr_slots[int_i].b_active) {
        SimShip *str_ship=&str_sim->arr_ships[int_i];
        str_ship->int_lives=1;str_ship->int_invulnerable=0;str_ship->int_respawn=0;
        str_ship->flt_vx=0;str_ship->flt_vy=0;
        str_sim->arr_asteroids[int_i]=(SimAsteroid){.b_active=true,.int_size=1,
            .int_id=++str_sim->int_next_id,.flt_x=str_ship->flt_x,.flt_y=str_ship->flt_y};
    }
    str_server->b_test_prepared=true;
    return true;
}

/* Session generation prevents a rapid leave/rejoin inheriting ship state. */
static void game_step(Server *str_server)
{
    uint16_t arr_keys[AST_PLAYERS] = {0};
    bool b_restart_requested = false;
    unsigned int int_active = 0;
    mutex_lock(&str_server->str_mutex);
    for (unsigned int int_i = 0; int_i < AST_PLAYERS; ++int_i) {
        const Slot *str_slot = &str_server->arr_slots[int_i];
        uint64_t int_session = str_slot->b_active ? str_slot->int_generation : 0;
        if (str_server->arr_sim_session[int_i] != int_session) {
            capture_score(str_server, int_i);
            sim_connect(&str_server->str_sim, int_i, false);
            sim_connect(&str_server->str_sim, int_i, str_slot->b_active);
            str_server->arr_sim_session[int_i] = int_session;
            str_server->arr_score_owner[int_i] = (ScoreOwner){.int_account_id = str_slot->int_account_id,
                .int_session = int_session, .int_identity = str_slot->int_identity};
        }
        if (str_slot->b_active && !str_slot->b_disconnected) ++int_active;
        arr_keys[int_i] = str_slot->b_active && !str_slot->b_disconnected ? str_slot->int_keys : 0;
        b_restart_requested |= (arr_keys[int_i] & AST_KEY_RESTART) != 0;
    }
    if (int_active >= 2) str_server->b_started = true;
    bool b_play = str_server->b_started && !str_server->b_paused && !str_server->b_timeout_end;
    bool b_fixture=b_play && prepare_gameover_test(str_server);
    uint64_t int_over_before=str_server->str_sim.str_counts.int_gameovers;
    uint64_t int_restart_before=str_server->str_sim.str_counts.int_restarts;
    if (str_server->str_sim.b_gameover && b_restart_requested) capture_scores(str_server);
    if (b_play) sim_tick(&str_server->str_sim, arr_keys);
    bool b_over=str_server->str_sim.str_counts.int_gameovers!=int_over_before;
    bool b_restart=str_server->str_sim.str_counts.int_restarts!=int_restart_before;
    if (b_over) capture_scores(str_server);
    if (b_restart) ++str_server->int_round;
    uint64_t int_tick=str_server->str_sim.int_tick;
    mutex_unlock(&str_server->str_mutex);
    if(b_fixture) printf("SERVER_TEST_SCENARIO name=gameover tick=%" PRIu64 "\n",int_tick);
    if(b_over) printf("GAME_EVENT gameover tick=%" PRIu64 "\n",int_tick);
    if(b_restart) printf("GAME_EVENT restart tick=%" PRIu64 "\n",int_tick);
}

/* Main owns lease expiry; the network worker owns receipt. Shared slots are locked. */
void server_expire(Server *str_server, uint64_t int_now)
{
    unsigned int arr_lost[AST_PLAYERS] = {0}; bool b_end = false;
    mutex_lock(&str_server->str_mutex);
    ++str_server->int_tick;
    str_server->b_paused = !str_server->b_started;
    str_server->int_reconnect_seconds = 0;
    for (unsigned int int_i = 0; int_i < AST_PLAYERS && !str_server->b_timeout_end; ++int_i) {
        Slot *str_slot = &str_server->arr_slots[int_i];
        if (!str_slot->b_active) continue;
        uint64_t int_age = int_now >= str_slot->int_seen ? int_now - str_slot->int_seen : 0;
        if (!str_slot->b_disconnected && int_age >= AST_LEASE_NS) {
            server_release_slot(str_server, int_i);
            arr_lost[int_i] = str_slot->int_identity; ++str_server->int_timeouts;
        }
        if (!str_slot->b_disconnected) continue;
        str_server->b_paused = true;
        if (int_age >= AST_RECONNECT_NS) {
            str_server->b_timeout_end = true; str_server->b_paused = false;
            str_server->int_reconnect_seconds = 0;
            str_server->int_round_end = int_now;
            str_server->str_sim.b_gameover = true;
            ++str_server->str_sim.str_counts.int_gameovers; b_end = true;
        } else {
            uint16_t int_left = (uint16_t)((AST_RECONNECT_NS - int_age + UINT64_C(999999999)) / UINT64_C(1000000000));
            if (!str_server->int_reconnect_seconds || int_left < str_server->int_reconnect_seconds)
                str_server->int_reconnect_seconds = int_left;
        }
    }
    mutex_unlock(&str_server->str_mutex);
    for (unsigned int int_i = 0; int_i < AST_PLAYERS; ++int_i) if (arr_lost[int_i]) {
        printf("SERVER_DISCONNECTED identity=%u id=%u grace_seconds=30\n", arr_lost[int_i], int_i + 1);
        audit_event(&str_server->str_audit, arr_lost[int_i], "DISCONNECTED", "reconnect-grace");
    }
    if (b_end) {
        puts("GAME_EVENT round-ended reason=absence-over-30s");
        audit_event(&str_server->str_audit, 0, "ROUND_END", "absence-over-30s");
    }
}

/* Called after join, so summary reads cannot race. No session tokens are printed. */
static void summary(const Server *str_server)
{
    const SimCounters *str_counts = &str_server->str_sim.str_counts;
    printf("SERVER_SUMMARY joins=%" PRIu64 " rejects=%" PRIu64 " leaves=%" PRIu64
           " timeouts=%" PRIu64 " invalid=%" PRIu64 " stale=%" PRIu64
           " peak=%u inputs=%" PRIu64 " rate_hits=%" PRIu64 " auth_failures=%" PRIuFAST64
           " middleware_events=%" PRIuFAST64 " moves=%" PRIu64 " shots=%" PRIu64
           " destroyed=%" PRIu64 " splits=%" PRIu64 " hits=%" PRIu64 " respawns=%" PRIu64
           " gameovers=%" PRIu64 " restarts=%" PRIu64 " wraps=%" PRIu64 " snapshots=%" PRIu64
           " points=%" PRIu64 " ticks=%" PRIu64 "\n", str_server->int_joins, str_server->int_rejects,
           str_server->int_leaves, str_server->int_timeouts, str_server->int_invalid,
           str_server->int_stale, str_server->int_peak, str_server->int_inputs, str_server->int_rate_hits,
           atomic_load(&str_server->str_audit.int_auth_failures), atomic_load(&str_server->str_audit.int_middleware_events),
           str_counts->int_moves, str_counts->int_shots, str_counts->int_destroyed, str_counts->int_splits,
           str_counts->int_hits, str_counts->int_respawns, str_counts->int_gameovers, str_counts->int_restarts,
           str_counts->int_wraps, str_server->int_snapshots, str_counts->int_points,str_server->str_sim.int_tick);
}

/* Start exactly one I/O worker, run finite ticks, then join before destroying state. */
int main(int int_argc, char **arr_argv)
{
    Options str_options = {0};
    Server str_server = {.str_mutex = PTHREAD_MUTEX_INITIALIZER};
    pthread_t str_thread;
    setvbuf(stdout, NULL, _IOLBF, 0);
    if (!options_parse(int_argc, arr_argv, &str_options, true)) { fprintf(stderr, "Invalid server options\n"); return 2; }
    if (signal(SIGINT, stop_signal) == SIG_ERR || signal(SIGTERM, stop_signal) == SIG_ERR) return 1;
    umask(0077);
    char arr_default_dir[PATH_MAX];
    if (!str_options.str_account_dir) {
        if (!account_private_dir(str_options.str_security_dir, "server", arr_default_dir)) {
            fputs("ACCOUNT_STORAGE_ERROR private directory unavailable\n", stderr);
            return 1;
        }
        str_options.str_account_dir = arr_default_dir;
    }
    if (!random_bytes(str_server.arr_boot_id, sizeof(str_server.arr_boot_id))) return 1;
    str_server.str_options = str_options;
    sim_init(&str_server.str_sim, UINT32_C(0xC5D3116));
    if (!audit_open(&str_server.str_audit, str_options.str_audit_file)) { fprintf(stderr, "audit_open failed\n"); return 1; }
    audit_event(&str_server.str_audit, 0, "START", "startup");
    if (atomic_load(&str_server.str_audit.b_failed)) { audit_close(&str_server.str_audit); return 1; }
    str_server.ptr_accounts = accounts_create(str_options.str_account_dir, &str_server.str_audit);
    if (!str_server.ptr_accounts) {
        fputs("ACCOUNT_STORAGE_ERROR startup failed\n", stderr);
        audit_close(&str_server.str_audit);
        return 1;
    }
    audit_dds_install(&str_server.str_audit);
    int int_result = pthread_create(&str_thread, NULL, server_worker, &str_server);
    if (int_result) {
        fprintf(stderr, "pthread_create: %s\n", strerror(int_result));
        accounts_destroy(str_server.ptr_accounts);
        audit_dds_uninstall(); audit_close(&str_server.str_audit); return 1;
    }
    while (!atomic_load(&str_server.b_ready) && !atomic_load(&str_server.b_stop) && !b_signal) sleep_ns(1000000);
    uint64_t int_start = monotonic_ns(), int_next = int_start;
    unsigned int int_ticks = str_options.int_ticks ? str_options.int_ticks : (unsigned int)(str_options.dbl_duration * AST_HZ);
    for (unsigned int int_tick = 0; int_tick < int_ticks && !b_signal && !atomic_load(&str_server.b_stop) &&
        !atomic_load(&str_server.str_audit.b_failed); ++int_tick) {
        server_expire(&str_server, monotonic_ns());
        game_step(&str_server);
        if (str_server.b_timeout_end && monotonic_ns() - str_server.int_round_end >= AST_END_DISPLAY_NS) break;
        int_next += UINT64_C(1000000000) / AST_HZ;
        uint64_t int_now = monotonic_ns();
        if (int_next > int_now) sleep_ns(int_next - int_now);
    }
    atomic_store(&str_server.b_stop, true);
    int_result = pthread_join(str_thread, NULL);
    if (int_result) { fprintf(stderr, "pthread_join: %s\n", strerror(int_result)); return 1; }
    capture_scores(&str_server);
    bool b_accounts_ok = accounts_destroy(str_server.ptr_accounts);
    summary(&str_server);
    audit_dds_uninstall();
    bool b_audit_ok = audit_close(&str_server.str_audit);
    int_result = pthread_mutex_destroy(&str_server.str_mutex);
    return int_result || !b_audit_ok || !b_accounts_ok || atomic_load(&str_server.b_failed) ? 1 : 0;
}
