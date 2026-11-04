/* Pointer-free snapshot handoff and serialization. Author: <team>/Codex, 2026-11-04. */
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
    bool b_gameover;
} Frame;
void frame_from_sim(Frame *str_frame, const Sim *str_sim);
void frame_to_wire(Frame *str_frame, Ast_Snapshot *str_snapshot);
void frame_from_wire(Frame *str_frame, const Ast_Snapshot *str_snapshot);
#endif
