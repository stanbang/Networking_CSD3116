/**
 * @file
 * @brief Pointer-free game frames and DDS snapshot conversion interfaces.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
#ifndef AST_FRAME_H
#define AST_FRAME_H
#include "common/protocol.h"
#include "common/sim.h"
typedef struct {
    Ast_Ship arr_ships[AST_PLAYERS];
    Ast_Bullet arr_bullets[AST_BULLETS];
    Ast_Asteroid arr_asteroids[AST_ASTEROIDS];
    uint32_t int_ships, int_bullets, int_asteroids;
    uint64_t int_tick, int_seq, int_received;
    uint16_t int_wave;
    bool b_gameover, b_paused, b_timeout_end;
    uint16_t int_reconnect_seconds;
} Frame;
/** @brief Copy renderable state out of the simulation. */
void frame_from_sim(Frame *str_frame, const Sim *str_sim);
/** @brief Serialize a game frame into the DDS snapshot fields. */
void frame_to_wire(Frame *str_frame, Ast_Snapshot *str_snapshot);
/** @brief Populate a local frame from a validated DDS snapshot. */
void frame_from_wire(Frame *str_frame, const Ast_Snapshot *str_snapshot);
#endif
