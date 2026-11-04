/* Independent behavior fixtures for authoritative gameplay.
 * Author: <team>/Codex tester, 2026-11-04. No network, renderer or wall-clock.
 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "common/sim.h"

/* Start isolated fixtures away from randomly generated waves. */
static Sim fixture(void)
{
    Sim str_sim;
    sim_init(&str_sim, 123);
    memset(str_sim.arr_asteroids, 0, sizeof(str_sim.arr_asteroids));
    sim_connect(&str_sim, 0, true);
    return str_sim;
}

/* Count visible pool members instead of trusting implementation counters. */
static unsigned int asteroids(const Sim *str_sim, unsigned int int_size)
{
    unsigned int int_count = 0;
    for (unsigned int int_i = 0; int_i < AST_ASTEROIDS; ++int_i)
        if (str_sim->arr_asteroids[int_i].b_active &&
            (!int_size || str_sim->arr_asteroids[int_i].int_size == int_size)) ++int_count;
    return int_count;
}

/* Contact fixtures exercise the public simulation, never private collision helpers. */
static void collision_pair(Sim *str_sim, unsigned int int_size)
{
    str_sim->arr_asteroids[0] = (SimAsteroid){.b_active=true, .flt_x=100, .flt_y=100,
        .int_size=(uint16_t)int_size, .int_id=1};
    str_sim->arr_bullets[0] = (SimBullet){.b_active=true, .flt_x=100, .flt_y=100,
        .int_owner=1, .int_life=10, .int_id=2};
}

/* Finite wrapping and post-tick speed must stay within world/physics bounds. */
static void movement_cases(void)
{
    assert(fabsf(sim_wrap(-1, 800)-799) < .001f);
    assert(fabsf(sim_wrap(2401, 800)-1) < .001f);
    Sim str_sim = fixture();
    SimShip *str_ship = &str_sim.arr_ships[0];
    str_ship->flt_x = 799; str_ship->flt_y = 200; str_ship->flt_vx = 10000;
    uint16_t arr_keys[AST_PLAYERS] = {0};
    sim_tick(&str_sim, arr_keys);
    assert(str_ship->flt_x >= 0 && str_ship->flt_x < AST_WIDTH);
    assert(hypotf(str_ship->flt_vx,str_ship->flt_vy) <= AST_SPEED_LIMIT+.001f);
    assert(str_sim.str_counts.int_wraps == 1 && str_sim.str_counts.int_moves == 1);
    float flt_angle = str_ship->flt_angle;
    arr_keys[0] = AST_KEY_RIGHT;
    sim_tick(&str_sim, arr_keys);
    assert(str_ship->flt_angle != flt_angle);
    puts("PASS sim wrap, speed limit and rotation");
}

/* Repeated held fire is cooldown-limited; the shooter has a capped fixed pool. */
static void fire_cases(void)
{
    Sim str_sim = fixture();
    uint16_t arr_keys[AST_PLAYERS] = {AST_KEY_FIRE};
    for (unsigned int int_i=0; int_i<AST_HZ; ++int_i) sim_tick(&str_sim,arr_keys);
    assert(str_sim.str_counts.int_shots == 5);
    for (unsigned int int_i=0; int_i<AST_BULLETS; ++int_i)
        str_sim.arr_bullets[int_i] = (SimBullet){.b_active=true, .int_life=90, .int_owner=1};
    str_sim.arr_ships[0].int_cooldown = 0;
    uint64_t int_before = str_sim.str_counts.int_shots;
    sim_tick(&str_sim,arr_keys);
    assert(str_sim.str_counts.int_shots == int_before);
    sim_connect(&str_sim,0,false);
    for (unsigned int int_i=0; int_i<AST_BULLETS; ++int_i) assert(!str_sim.arr_bullets[int_i].b_active);
    puts("PASS sim fire cadence, full bullet pool and leave cleanup");
}

/* A real collision consumes one bullet, splits once and credits only its owner. */
static void destruction_cases(void)
{
    Sim str_sim = fixture();
    collision_pair(&str_sim,3);
    uint16_t arr_keys[AST_PLAYERS] = {0};
    sim_tick(&str_sim,arr_keys);
    assert(!str_sim.arr_bullets[0].b_active);
    assert(str_sim.str_counts.int_destroyed==1 && str_sim.str_counts.int_splits==1);
    assert(asteroids(&str_sim,2)==2 && asteroids(&str_sim,0)==2);
    assert(str_sim.arr_ships[0].int_score==20 && str_sim.arr_ships[1].int_score==0);
    str_sim=fixture(); collision_pair(&str_sim,1);
    str_sim.arr_bullets[0].flt_x=799; str_sim.arr_asteroids[0].flt_x=1;
    sim_tick(&str_sim,arr_keys);
    assert(str_sim.str_counts.int_destroyed==1 && asteroids(&str_sim,0)==0);
    assert(str_sim.arr_ships[0].int_score==100 && str_sim.str_counts.int_splits==0);
    puts("PASS sim splitting, owner score and seam collision");
}

/* Contact costs one life, waits for respawn, and cannot hurt invulnerable ships. */
static void damage_cases(void)
{
    Sim str_sim=fixture();
    SimShip *str_ship=&str_sim.arr_ships[0];
    str_ship->int_invulnerable=0;
    str_sim.arr_asteroids[0]=(SimAsteroid){.b_active=true,.int_size=1,
        .flt_x=str_ship->flt_x,.flt_y=str_ship->flt_y};
    str_sim.arr_asteroids[1]=str_sim.arr_asteroids[0];
    uint16_t arr_keys[AST_PLAYERS]={0};
    sim_tick(&str_sim,arr_keys);
    assert(str_ship->int_lives==AST_LIVES-1 && str_sim.str_counts.int_hits==1);
    assert(str_ship->int_respawn==AST_RESPAWN_TICKS);
    for (unsigned int int_i=0; int_i<AST_RESPAWN_TICKS-1; ++int_i) sim_tick(&str_sim,arr_keys);
    assert(str_sim.str_counts.int_respawns==0 && str_ship->int_respawn==1);
    sim_tick(&str_sim,arr_keys);
    assert(str_sim.str_counts.int_respawns==1 && str_ship->int_respawn==0);
    for (unsigned int int_i=0; int_i<10; ++int_i) sim_tick(&str_sim,arr_keys);
    assert(str_ship->int_lives==AST_LIVES-1 && str_ship->int_invulnerable>0);
    puts("PASS sim damage, exact respawn delay and invulnerability");
}

/* Defeat is authoritative; explicit restart restores game state, not evidence counters. */
static void restart_cases(void)
{
    Sim str_sim=fixture();
    str_sim.arr_ships[0].int_lives=1; str_sim.arr_ships[0].int_invulnerable=0;
    str_sim.arr_ships[0].int_score=77;
    str_sim.arr_asteroids[0]=(SimAsteroid){.b_active=true,.int_size=1,
        .flt_x=str_sim.arr_ships[0].flt_x,.flt_y=str_sim.arr_ships[0].flt_y};
    uint16_t arr_keys[AST_PLAYERS]={0};
    sim_tick(&str_sim,arr_keys); sim_tick(&str_sim,arr_keys);
    assert(str_sim.b_gameover && str_sim.str_counts.int_gameovers==1);
    assert(str_sim.arr_ships[0].int_lives==0);
    arr_keys[0]=AST_KEY_RESTART; sim_tick(&str_sim,arr_keys);
    assert(!str_sim.b_gameover && str_sim.str_counts.int_restarts==1);
    assert(str_sim.arr_ships[0].int_lives==AST_LIVES && str_sim.arr_ships[0].int_score==0);
    assert(str_sim.arr_ships[0].int_invulnerable>0 && asteroids(&str_sim,0)>0);
    assert(str_sim.str_counts.int_gameovers==1 && str_sim.int_wave==1);
    puts("PASS sim game-over and explicit restart");
}

/* Simultaneous splitting at capacity is bounded and cannot touch adjacent storage. */
static void capacity_cases(void)
{
    struct { uint64_t int_before; Sim str_sim; uint64_t int_after; } str_guard = {
        .int_before=UINT64_C(0x1122334455667788), .str_sim=fixture(),
        .int_after=UINT64_C(0x8877665544332211)};
    for (unsigned int int_i=0; int_i<AST_ASTEROIDS; ++int_i)
        str_guard.str_sim.arr_asteroids[int_i]=(SimAsteroid){.b_active=true,.int_size=3,
            .flt_x=600,.flt_y=500,.int_id=int_i+1};
    collision_pair(&str_guard.str_sim,3);
    uint16_t arr_keys[AST_PLAYERS]={0}; sim_tick(&str_guard.str_sim,arr_keys);
    assert(asteroids(&str_guard.str_sim,0)==AST_ASTEROIDS);
    assert(asteroids(&str_guard.str_sim,2)==1 && str_guard.str_sim.str_counts.int_splits==1);
    assert(str_guard.int_before==UINT64_C(0x1122334455667788));
    assert(str_guard.int_after==UINT64_C(0x8877665544332211));
    puts("PASS sim capped split pool and guard words");
}

/* Each behavioral assertion aborts nonzero; NDEBUG must not be used for this target. */
int main(void)
{
    movement_cases(); fire_cases(); destruction_cases(); damage_cases(); restart_cases(); capacity_cases();
    puts("PASS independent simulation behavior fixtures");
    return 0;
}
