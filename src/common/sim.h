/**
 * @file
 * @brief Simulation state, object pools and game update interfaces.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
#ifndef AST_SIM_H
#define AST_SIM_H
#include <stdbool.h>
#include <stdint.h>
#include "common/config.h"
typedef struct {
    float flt_x, flt_y, flt_vx, flt_vy, flt_angle;
    bool b_active;
    uint16_t int_lives, int_respawn, int_invulnerable, int_cooldown;
    uint32_t int_score;
    double dbl_distance;
} SimShip;
typedef struct {
    float flt_x, flt_y, flt_vx, flt_vy;
    bool b_active;
    uint16_t int_owner, int_life;
    uint32_t int_id;
} SimBullet;
typedef struct {
    float flt_x, flt_y, flt_vx, flt_vy;
    bool b_active;
    uint16_t int_size;
    uint32_t int_id;
} SimAsteroid;
typedef struct {
    uint64_t int_moves, int_shots, int_destroyed, int_splits, int_hits;
    uint64_t int_respawns, int_gameovers, int_restarts, int_wraps;
    uint64_t int_points;
} SimCounters;
typedef struct {
    SimShip arr_ships[AST_PLAYERS];
    SimBullet arr_bullets[AST_BULLETS];
    SimAsteroid arr_asteroids[AST_ASTEROIDS];
    SimCounters str_counts;
    uint64_t int_tick;
    uint32_t int_rng, int_next_id;
    uint16_t int_wave, int_wave_delay;
    bool b_gameover;
} Sim;
/** @brief Initialize the simulation with a deterministic random seed. */
void sim_init(Sim *str_sim, uint32_t int_seed);
/** @brief Activate or remove a player in the specified simulation slot. */
void sim_connect(Sim *str_sim, unsigned int int_slot, bool b_active);
/** @brief Advance the simulation by one tick using the supplied player inputs. */
void sim_tick(Sim *str_sim, const uint16_t arr_keys[AST_PLAYERS]);
/** @brief Return the collision radius for an asteroid size. */
float sim_radius(unsigned int int_size);
/** @brief Wrap a coordinate into the arena extent. */
float sim_wrap(float flt_value, float flt_extent);
/** @brief Choose bot input for the given simulation slot. */
uint16_t sim_hunter_keys(const Sim *str_sim, unsigned int int_slot);
#endif
