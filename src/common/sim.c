/**
 * @file
 * @brief Deterministic fixed-tick game simulation with fixed-size object pools.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
#include <math.h>
#include <string.h>
#include "common/sim.h"

/* Deterministic simulation randomness is separate from OpenSSL protocol secrets. */
static uint32_t next_random(Sim *str_sim)
{
    uint32_t int_value = str_sim->int_rng;
    int_value ^= int_value << 13; int_value ^= int_value >> 17; int_value ^= int_value << 5;
    str_sim->int_rng = int_value;
    return int_value;
}

/* fmod handles even a large step without an unbounded placement/wrap loop. */
float sim_wrap(float flt_value, float flt_extent)
{
    float flt_wrapped = fmodf(flt_value, flt_extent);
    return flt_wrapped < 0 ? flt_wrapped + flt_extent : flt_wrapped;
}

/* Size is internal and validated at the network boundary. */
float sim_radius(unsigned int int_size)
{
    const float arr_radii[] = {0, 11, 20, 32};
    return int_size <= 3 ? arr_radii[int_size] : 0;
}

/* Toroidal distance also detects collisions across arena edges. */
static bool overlaps(float flt_x1, float flt_y1, float flt_x2, float flt_y2, float flt_radius)
{
    float flt_dx = fabsf(flt_x1 - flt_x2), flt_dy = fabsf(flt_y1 - flt_y2);
    flt_dx = fminf(flt_dx, AST_WIDTH - flt_dx);
    flt_dy = fminf(flt_dy, AST_HEIGHT - flt_dy);
    return flt_dx * flt_dx + flt_dy * flt_dy <= flt_radius * flt_radius;
}

/* Respawn uses fixed separated positions, hence no adversarial placement loop. */
static void spawn_ship(SimShip *str_ship, unsigned int int_slot)
{
    str_ship->flt_x = 180.0f + (float)int_slot * 110.0f;
    str_ship->flt_y = AST_HEIGHT / 2.0f;
    str_ship->flt_vx = str_ship->flt_vy = 0;
    str_ship->flt_angle = AST_PI * 1.5f;
    str_ship->int_respawn = 0;
    str_ship->int_invulnerable = AST_INVULNERABLE_TICKS;
    str_ship->int_cooldown = 0;
}

/* Joining/leaving changes only a bounded slot and its owned bullet pool. */
void sim_connect(Sim *str_sim, unsigned int int_slot, bool b_active)
{
    if (int_slot >= AST_PLAYERS || str_sim->arr_ships[int_slot].b_active == b_active) return;
    str_sim->arr_ships[int_slot] = (SimShip){.b_active = b_active, .int_lives = AST_LIVES};
    if (b_active) spawn_ship(&str_sim->arr_ships[int_slot], int_slot);
    else for (unsigned int int_i = 0; int_i < AST_BULLETS; ++int_i)
        if (str_sim->arr_bullets[int_i].int_owner == int_slot + 1) str_sim->arr_bullets[int_i].b_active = false;
}

/* At pool capacity a split child is omitted, never allocated dynamically. */
static void add_asteroid(Sim *str_sim, float flt_x, float flt_y, unsigned int int_size)
{
    for (unsigned int int_i = 0; int_i < AST_ASTEROIDS; ++int_i) if (!str_sim->arr_asteroids[int_i].b_active) {
        float flt_angle = (float)(next_random(str_sim) % 6284) / 1000.0f;
        float flt_speed = 30.0f + (float)(next_random(str_sim) % 41);
        str_sim->arr_asteroids[int_i] = (SimAsteroid){.b_active = true, .flt_x = flt_x, .flt_y = flt_y,
            .flt_vx = cosf(flt_angle) * flt_speed, .flt_vy = sinf(flt_angle) * flt_speed,
            .int_size = (uint16_t)int_size, .int_id = ++str_sim->int_next_id};
        return;
    }
}

/* Waves spawn at edges, away from the central player spawn row. */
static void new_wave(Sim *str_sim)
{
    ++str_sim->int_wave;
    unsigned int int_count = 3u + str_sim->int_wave;
    if (int_count > 12) int_count = 12;
    for (unsigned int int_i = 0; int_i < int_count; ++int_i)
        add_asteroid(str_sim, (float)(next_random(str_sim) % AST_WIDTH), int_i % 2 ? 35.0f : AST_HEIGHT - 35.0f, 3);
    str_sim->int_wave_delay = AST_WAVE_DELAY;
}

/* Initial state is fully initialized, including padding never sent as raw bytes. */
void sim_init(Sim *str_sim, uint32_t int_seed)
{
    *str_sim = (Sim){.int_rng = int_seed ? int_seed : 1};
    new_wave(str_sim);
}

/* Fire produces one pooled bullet per cooldown, independent of received packet rate. */
static void fire(Sim *str_sim, unsigned int int_slot)
{
    SimShip *str_ship = &str_sim->arr_ships[int_slot];
    if (str_ship->int_cooldown) return;
    for (unsigned int int_i = 0; int_i < AST_BULLETS; ++int_i) if (!str_sim->arr_bullets[int_i].b_active) {
        float flt_dx = cosf(str_ship->flt_angle), flt_dy = sinf(str_ship->flt_angle);
        str_sim->arr_bullets[int_i] = (SimBullet){.b_active = true, .int_owner = (uint16_t)(int_slot + 1),
            .int_life = AST_BULLET_TICKS, .int_id = ++str_sim->int_next_id,
            .flt_x = sim_wrap(str_ship->flt_x + flt_dx * 15, AST_WIDTH),
            .flt_y = sim_wrap(str_ship->flt_y + flt_dy * 15, AST_HEIGHT),
            .flt_vx = str_ship->flt_vx + flt_dx * AST_BULLET_SPEED,
            .flt_vy = str_ship->flt_vy + flt_dy * AST_BULLET_SPEED};
        str_ship->int_cooldown = AST_FIRE_TICKS;
        ++str_sim->str_counts.int_shots;
        return;
    }
}

/* Integrate once per authoritative tick; angular keys may cancel one another. */
static void move_ship(Sim *str_sim, unsigned int int_slot, uint16_t int_keys)
{
    SimShip *str_ship = &str_sim->arr_ships[int_slot];
    const float flt_dt = 1.0f / AST_HZ;
    int int_turn = ((int_keys & AST_KEY_RIGHT) != 0) - ((int_keys & AST_KEY_LEFT) != 0);
    str_ship->flt_angle = sim_wrap(str_ship->flt_angle + int_turn * AST_TURN_SPEED * flt_dt, 2 * AST_PI);
    if (int_keys & AST_KEY_THRUST) {
        str_ship->flt_vx += cosf(str_ship->flt_angle) * AST_THRUST * flt_dt;
        str_ship->flt_vy += sinf(str_ship->flt_angle) * AST_THRUST * flt_dt;
    }
    str_ship->flt_vx *= AST_FRICTION; str_ship->flt_vy *= AST_FRICTION;
    float flt_speed = hypotf(str_ship->flt_vx, str_ship->flt_vy);
    if (flt_speed > AST_SPEED_LIMIT) {
        str_ship->flt_vx *= AST_SPEED_LIMIT / flt_speed; str_ship->flt_vy *= AST_SPEED_LIMIT / flt_speed;
    }
    float flt_x = str_ship->flt_x + str_ship->flt_vx * flt_dt, flt_y = str_ship->flt_y + str_ship->flt_vy * flt_dt;
    if (flt_x < 0 || flt_x >= AST_WIDTH || flt_y < 0 || flt_y >= AST_HEIGHT) ++str_sim->str_counts.int_wraps;
    str_ship->flt_x = sim_wrap(flt_x, AST_WIDTH); str_ship->flt_y = sim_wrap(flt_y, AST_HEIGHT);
    str_ship->dbl_distance += fminf(flt_speed, AST_SPEED_LIMIT) * flt_dt;
    if (flt_speed > 0.01f) ++str_sim->str_counts.int_moves;
    if (int_keys & AST_KEY_FIRE) fire(str_sim, int_slot);
}

/* Waiting and invulnerability are ticks, so packet flood cannot accelerate them. */
static void ships_tick(Sim *str_sim, const uint16_t arr_keys[AST_PLAYERS])
{
    for (unsigned int int_i = 0; int_i < AST_PLAYERS; ++int_i) {
        SimShip *str_ship = &str_sim->arr_ships[int_i];
        if (!str_ship->b_active || !str_ship->int_lives) continue;
        if (str_ship->int_respawn) {
            if (--str_ship->int_respawn == 0) { spawn_ship(str_ship, int_i); ++str_sim->str_counts.int_respawns; }
            continue;
        }
        if (str_ship->int_invulnerable) --str_ship->int_invulnerable;
        if (str_ship->int_cooldown) --str_ship->int_cooldown;
        move_ship(str_sim, int_i, arr_keys[int_i]);
    }
}

/* Movement uses toroidal coordinates for all entity types. */
static void entities_tick(Sim *str_sim)
{
    for (unsigned int int_i = 0; int_i < AST_ASTEROIDS; ++int_i) {
        SimAsteroid *str_asteroid = &str_sim->arr_asteroids[int_i];
        if (!str_asteroid->b_active) continue;
        str_asteroid->flt_x = sim_wrap(str_asteroid->flt_x + str_asteroid->flt_vx / AST_HZ, AST_WIDTH);
        str_asteroid->flt_y = sim_wrap(str_asteroid->flt_y + str_asteroid->flt_vy / AST_HZ, AST_HEIGHT);
    }
    for (unsigned int int_i = 0; int_i < AST_BULLETS; ++int_i) {
        SimBullet *str_bullet = &str_sim->arr_bullets[int_i];
        if (!str_bullet->b_active) continue;
        if (--str_bullet->int_life == 0) { str_bullet->b_active = false; continue; }
        str_bullet->flt_x = sim_wrap(str_bullet->flt_x + str_bullet->flt_vx / AST_HZ, AST_WIDTH);
        str_bullet->flt_y = sim_wrap(str_bullet->flt_y + str_bullet->flt_vy / AST_HZ, AST_HEIGHT);
    }
}

/* Kill/split consumes the bullet once and awards only its validated owner slot. */
static void destroy_asteroid(Sim *str_sim, SimBullet *str_bullet, unsigned int int_asteroid)
{
    SimAsteroid str_old = str_sim->arr_asteroids[int_asteroid];
    str_sim->arr_asteroids[int_asteroid].b_active = false;
    str_bullet->b_active = false;
    ++str_sim->str_counts.int_destroyed;
    const uint32_t arr_score[] = {0,100,50,20};
    str_sim->arr_ships[str_bullet->int_owner - 1].int_score += arr_score[str_old.int_size];
    str_sim->str_counts.int_points += arr_score[str_old.int_size];
    if (str_old.int_size > 1) {
        add_asteroid(str_sim, str_old.flt_x, str_old.flt_y, str_old.int_size - 1);
        add_asteroid(str_sim, str_old.flt_x, str_old.flt_y, str_old.int_size - 1);
        ++str_sim->str_counts.int_splits;
    }
}

/* Caps make this simple broad phase bounded; profiling can replace it in P5. */
static void bullet_collisions(Sim *str_sim)
{
    for (unsigned int int_i = 0; int_i < AST_BULLETS; ++int_i) {
        SimBullet *str_bullet = &str_sim->arr_bullets[int_i];
        if (!str_bullet->b_active) continue;
        for (unsigned int int_j = 0; int_j < AST_ASTEROIDS; ++int_j) {
            const SimAsteroid *str_asteroid = &str_sim->arr_asteroids[int_j];
            if (str_asteroid->b_active && overlaps(str_bullet->flt_x, str_bullet->flt_y,
                str_asteroid->flt_x, str_asteroid->flt_y, sim_radius(str_asteroid->int_size) + 2)) {
                destroy_asteroid(str_sim, str_bullet, int_j); break;
            }
        }
    }
}

/* Friendly fire is off. One asteroid contact costs at most one life per tick. */
static void ship_collisions(Sim *str_sim)
{
    for (unsigned int int_i = 0; int_i < AST_PLAYERS; ++int_i) {
        SimShip *str_ship = &str_sim->arr_ships[int_i];
        if (!str_ship->b_active || !str_ship->int_lives || str_ship->int_respawn || str_ship->int_invulnerable) continue;
        for (unsigned int int_j = 0; int_j < AST_ASTEROIDS; ++int_j) {
            const SimAsteroid *str_asteroid = &str_sim->arr_asteroids[int_j];
            if (str_asteroid->b_active && overlaps(str_ship->flt_x, str_ship->flt_y,
                str_asteroid->flt_x, str_asteroid->flt_y, sim_radius(str_asteroid->int_size) + AST_SHIP_RADIUS)) {
                --str_ship->int_lives; ++str_sim->str_counts.int_hits;
                str_ship->int_respawn = str_ship->int_lives ? AST_RESPAWN_TICKS : 0;
                str_ship->flt_vx = str_ship->flt_vy = 0;
                break;
            }
        }
    }
}

/* Preserve cumulative evidence counters and connections across an explicit restart. */
static void restart(Sim *str_sim)
{
    memset(str_sim->arr_bullets, 0, sizeof(str_sim->arr_bullets));
    memset(str_sim->arr_asteroids, 0, sizeof(str_sim->arr_asteroids));
    for (unsigned int int_i = 0; int_i < AST_PLAYERS; ++int_i) if (str_sim->arr_ships[int_i].b_active) {
        str_sim->arr_ships[int_i].int_lives = AST_LIVES;
        str_sim->arr_ships[int_i].int_score = 0;
        spawn_ship(&str_sim->arr_ships[int_i], int_i);
    }
    str_sim->b_gameover = false; str_sim->int_wave = 0;
    ++str_sim->str_counts.int_restarts;
    new_wave(str_sim);
}

/* Every call advances one60Hz step. No input count or wall time enters physics. */
void sim_tick(Sim *str_sim, const uint16_t arr_keys[AST_PLAYERS])
{
    ++str_sim->int_tick;
    bool b_any = false, b_alive = false, b_restart = false, b_asteroids = false;
    for (unsigned int int_i = 0; int_i < AST_PLAYERS; ++int_i) if (str_sim->arr_ships[int_i].b_active) {
        b_any = true; b_alive |= str_sim->arr_ships[int_i].int_lives > 0;
        b_restart |= (arr_keys[int_i] & AST_KEY_RESTART) != 0;
    }
    if (str_sim->b_gameover) { if (b_restart) restart(str_sim); return; }
    if (!b_any) return;
    if (!b_alive) { str_sim->b_gameover = true; ++str_sim->str_counts.int_gameovers; return; }
    ships_tick(str_sim, arr_keys); entities_tick(str_sim);
    bullet_collisions(str_sim); ship_collisions(str_sim);
    for (unsigned int int_i = 0; int_i < AST_ASTEROIDS; ++int_i) b_asteroids |= str_sim->arr_asteroids[int_i].b_active;
    if (!b_asteroids && str_sim->int_wave_delay && --str_sim->int_wave_delay == 0) new_wave(str_sim);
}

/* Headless hunter aims at the nearest toroidal asteroid; it obeys ordinary inputs. */
uint16_t sim_hunter_keys(const Sim *str_sim, unsigned int int_slot)
{
    if (int_slot >= AST_PLAYERS) return 0;
    const SimShip *str_ship = &str_sim->arr_ships[int_slot];
    if (str_sim->b_gameover) return AST_KEY_RESTART;
    float flt_best = 1e20f, flt_target = str_ship->flt_angle;
    for (unsigned int int_i = 0; int_i < AST_ASTEROIDS; ++int_i) {
        const SimAsteroid *str_asteroid = &str_sim->arr_asteroids[int_i];
        if (!str_asteroid->b_active) continue;
        float flt_dx = str_asteroid->flt_x - str_ship->flt_x, flt_dy = str_asteroid->flt_y - str_ship->flt_y;
        if (flt_dx > AST_WIDTH/2) flt_dx -= AST_WIDTH; else if (flt_dx < -AST_WIDTH/2) flt_dx += AST_WIDTH;
        if (flt_dy > AST_HEIGHT/2) flt_dy -= AST_HEIGHT; else if (flt_dy < -AST_HEIGHT/2) flt_dy += AST_HEIGHT;
        float flt_distance = flt_dx*flt_dx + flt_dy*flt_dy;
        if (flt_distance < flt_best) { flt_best = flt_distance; flt_target = atan2f(flt_dy,flt_dx); }
    }
    float flt_delta = remainderf(flt_target - str_ship->flt_angle, 2*AST_PI);
    return AST_KEY_FIRE | (fabsf(flt_delta)>0.08f ? (flt_delta>0 ? AST_KEY_RIGHT : AST_KEY_LEFT) : 0) |
        (str_sim->int_tick % 180 < 90 ? AST_KEY_THRUST : 0);
}
