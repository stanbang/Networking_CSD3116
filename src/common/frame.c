/* Explicit fields prevent raw padding/pointers leaking to DDS. Author: <team>/Codex, 2026-11-04. */
#include <math.h>
#include <string.h>
#include "common/frame.h"

/* Quantize to hundredths; wrapping ensures values stay inside validated arena. */
static int32_t coordinate(float flt_value, float flt_extent)
{
    int32_t int_result = (int32_t)(sim_wrap(flt_value, flt_extent) * 100.0f);
    return int_result >= (int32_t)(flt_extent * 100) ? 0 : int_result;
}

/* Caller holds server mutex; local frame storage owns every backing array. */
void frame_from_sim(Frame *str_frame, const Sim *str_sim)
{
    *str_frame = (Frame){.int_tick = str_sim->int_tick, .int_wave = str_sim->int_wave,
                         .b_gameover = str_sim->b_gameover};
    for (unsigned int int_i = 0; int_i < AST_PLAYERS; ++int_i) {
        const SimShip *str_ship = &str_sim->arr_ships[int_i];
        if (!str_ship->b_active) continue;
        str_frame->arr_ships[str_frame->int_ships++] = (Ast_Ship){.int_player_id = (uint16_t)(int_i+1),
            .int_x = coordinate(str_ship->flt_x,AST_WIDTH), .int_y = coordinate(str_ship->flt_y,AST_HEIGHT),
            .int_angle = (uint16_t)(sim_wrap(str_ship->flt_angle,2*AST_PI) * (65535.0f/(2*AST_PI))),
            .int_vx = (int16_t)(str_ship->flt_vx * 100), .int_vy = (int16_t)(str_ship->flt_vy * 100),
            .int_lives = str_ship->int_lives, .int_respawn = str_ship->int_respawn,
            .int_invulnerable = str_ship->int_invulnerable, .int_score = str_ship->int_score,
            .int_distance_mm = (uint64_t)(str_ship->dbl_distance * 1000)};
    }
    for (unsigned int int_i = 0; int_i < AST_BULLETS; ++int_i) {
        const SimBullet *str_bullet = &str_sim->arr_bullets[int_i];
        if (!str_bullet->b_active) continue;
        str_frame->arr_bullets[str_frame->int_bullets++] = (Ast_Bullet){.int_id = str_bullet->int_id,
            .int_owner = str_bullet->int_owner, .int_x = coordinate(str_bullet->flt_x,AST_WIDTH),
            .int_y = coordinate(str_bullet->flt_y,AST_HEIGHT)};
    }
    for (unsigned int int_i = 0; int_i < AST_ASTEROIDS; ++int_i) {
        const SimAsteroid *str_asteroid = &str_sim->arr_asteroids[int_i];
        if (!str_asteroid->b_active) continue;
        str_frame->arr_asteroids[str_frame->int_asteroids++] = (Ast_Asteroid){.int_id = str_asteroid->int_id,
            .int_size = str_asteroid->int_size, .int_x = coordinate(str_asteroid->flt_x,AST_WIDTH),
            .int_y = coordinate(str_asteroid->flt_y,AST_HEIGHT)};
    }
}

/* Loan-free serialization aliases only local frame arrays during synchronous write. */
void frame_to_wire(Frame *str_frame, Ast_Snapshot *str_snapshot)
{
    *str_snapshot = (Ast_Snapshot){.int_version=AST_VERSION,.int_seq=str_frame->int_seq,
        .int_tick=str_frame->int_tick,.int_wave=str_frame->int_wave,.b_gameover=str_frame->b_gameover,
        .b_paused=str_frame->b_paused,.b_timeout_end=str_frame->b_timeout_end,
        .int_reconnect_seconds=str_frame->int_reconnect_seconds,
        .arr_ships={._maximum=AST_PLAYERS,._length=str_frame->int_ships,._buffer=str_frame->arr_ships},
        .arr_bullets={._maximum=AST_BULLETS,._length=str_frame->int_bullets,._buffer=str_frame->arr_bullets},
        .arr_asteroids={._maximum=AST_ASTEROIDS,._length=str_frame->int_asteroids,._buffer=str_frame->arr_asteroids}};
}

/* Call only after snapshot_valid; copy before returning the middleware loan. */
void frame_from_wire(Frame *str_frame, const Ast_Snapshot *str_snapshot)
{
    *str_frame = (Frame){.int_tick=str_snapshot->int_tick,.int_seq=str_snapshot->int_seq,
        .int_received=monotonic_ns(),.int_wave=str_snapshot->int_wave,.b_gameover=str_snapshot->b_gameover,
        .b_paused=str_snapshot->b_paused,.b_timeout_end=str_snapshot->b_timeout_end,
        .int_reconnect_seconds=str_snapshot->int_reconnect_seconds,
        .int_ships=str_snapshot->arr_ships._length,.int_bullets=str_snapshot->arr_bullets._length,
        .int_asteroids=str_snapshot->arr_asteroids._length};
    if (str_frame->int_ships) memcpy(str_frame->arr_ships,str_snapshot->arr_ships._buffer,str_frame->int_ships*sizeof(Ast_Ship));
    if (str_frame->int_bullets) memcpy(str_frame->arr_bullets,str_snapshot->arr_bullets._buffer,str_frame->int_bullets*sizeof(Ast_Bullet));
    if (str_frame->int_asteroids) memcpy(str_frame->arr_asteroids,str_snapshot->arr_asteroids._buffer,str_frame->int_asteroids*sizeof(Ast_Asteroid));
}
