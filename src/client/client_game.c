/**
 * @file
 * @brief Snapshot interpolation and bot input based on the latest game state.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
#include <string.h>
#include "client/client.h"

/* Copy independent frames while locked; renderer holds no DDS-loan pointers. */
bool client_frames(Client *str_client, Frame *str_from, Frame *str_to, float *flt_alpha)
{
    uint64_t int_now = monotonic_ns();
    uint64_t int_target = int_now > AST_INTERPOLATION_NS ? int_now - AST_INTERPOLATION_NS : 0;
    mutex_lock(&str_client->str_mutex);
    if (!str_client->int_frames) { mutex_unlock(&str_client->str_mutex); return false; }
    unsigned int int_first = (str_client->int_head + AST_FRAME_HISTORY - str_client->int_frames) % AST_FRAME_HISTORY;
    *str_from = str_client->arr_frames[int_first];
    *str_to = *str_from;
    for (unsigned int int_i = 0; int_i < str_client->int_frames; ++int_i) {
        const Frame *str_frame = &str_client->arr_frames[(int_first + int_i) % AST_FRAME_HISTORY];
        *str_to = *str_frame;
        if (str_frame->int_received >= int_target) break;
        *str_from = *str_frame;
    }
    mutex_unlock(&str_client->str_mutex);
    uint64_t int_span = str_to->int_received - str_from->int_received;
    *flt_alpha = int_span && int_target > str_from->int_received ?
        (float)(int_target - str_from->int_received) / (float)int_span : 1;
    if (*flt_alpha > 1) *flt_alpha = 1;
    return true;
}

/* Bots read the same public state as humans and produce ordinary key states. */
uint16_t client_bot_keys(Client *str_client)
{
    const char *str_bot = str_client->str_options.str_bot;
    if (strcmp(str_bot,"idle") == 0) return 0;
    if (strcmp(str_bot,"straight") == 0) return AST_KEY_THRUST;
    if (strcmp(str_bot,"circle") == 0) return AST_KEY_THRUST | AST_KEY_RIGHT | AST_KEY_FIRE | AST_KEY_RESTART;
    Sim str_sim = {0}; Frame str_frame = {0};
    mutex_lock(&str_client->str_mutex);
    if (str_client->int_frames) str_frame = str_client->arr_frames[(str_client->int_head + AST_FRAME_HISTORY - 1) % AST_FRAME_HISTORY];
    mutex_unlock(&str_client->str_mutex);
    str_sim.b_gameover = str_frame.b_gameover; str_sim.int_tick = str_frame.int_tick;
    for (unsigned int int_i = 0; int_i < str_frame.int_ships; ++int_i) {
        const Ast_Ship *str_ship = &str_frame.arr_ships[int_i];
        SimShip *str_out = &str_sim.arr_ships[str_ship->int_player_id - 1];
        str_out->flt_x = str_ship->int_x / 100.0f; str_out->flt_y = str_ship->int_y / 100.0f;
        str_out->flt_angle = str_ship->int_angle * (2*AST_PI/65535.0f);
    }
    for (unsigned int int_i = 0; int_i < str_frame.int_asteroids; ++int_i) {
        str_sim.arr_asteroids[int_i] = (SimAsteroid){.b_active=true,
            .flt_x=str_frame.arr_asteroids[int_i].int_x/100.0f,.flt_y=str_frame.arr_asteroids[int_i].int_y/100.0f};
    }
    unsigned int int_player = atomic_load(&str_client->int_player_id);
    return int_player ? sim_hunter_keys(&str_sim,int_player-1) : 0;
}
