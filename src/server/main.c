/* Fixed-tick server lifetime. Author: <team>, Codex assistance. Date: 2026-11-03. */
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "server/server.h"
static volatile sig_atomic_t b_signal = 0;

/* Signal handlers touch only sig_atomic_t state. */
static void stop_signal(int int_signal) { (void)int_signal; b_signal = 1; }

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
    mutex_lock(&str_server->str_mutex);
    for (unsigned int int_i = 0; int_i < AST_PLAYERS; ++int_i) {
        const Slot *str_slot = &str_server->arr_slots[int_i];
        uint64_t int_session = str_slot->b_active ? str_slot->int_session : 0;
        if (str_server->arr_sim_session[int_i] != int_session) {
            sim_connect(&str_server->str_sim, int_i, false);
            sim_connect(&str_server->str_sim, int_i, str_slot->b_active);
            str_server->arr_sim_session[int_i] = int_session;
        }
        arr_keys[int_i] = str_slot->b_active ? str_slot->int_keys : 0;
    }
    bool b_fixture=prepare_gameover_test(str_server);
    uint64_t int_over_before=str_server->str_sim.str_counts.int_gameovers;
    uint64_t int_restart_before=str_server->str_sim.str_counts.int_restarts;
    sim_tick(&str_server->str_sim, arr_keys);
    bool b_over=str_server->str_sim.str_counts.int_gameovers!=int_over_before;
    bool b_restart=str_server->str_sim.str_counts.int_restarts!=int_restart_before;
    uint64_t int_tick=str_server->str_sim.int_tick;
    mutex_unlock(&str_server->str_mutex);
    if(b_fixture) printf("SERVER_TEST_SCENARIO name=gameover tick=%" PRIu64 "\n",int_tick);
    if(b_over) printf("GAME_EVENT gameover tick=%" PRIu64 "\n",int_tick);
    if(b_restart) printf("GAME_EVENT restart tick=%" PRIu64 "\n",int_tick);
}

/* Main owns lease expiry; the network worker owns receipt. Shared slots are locked. */
void server_expire(Server *str_server, uint64_t int_now)
{
    unsigned int arr_expired[AST_PLAYERS] = {0};
    mutex_lock(&str_server->str_mutex);
    ++str_server->int_tick;
    for (unsigned int int_i = 0; int_i < AST_PLAYERS; ++int_i) {
        Slot *str_slot = &str_server->arr_slots[int_i];
        if (str_slot->b_active && lease_expired(int_now, str_slot->int_seen)) {
            arr_expired[int_i] = str_slot->int_identity;
            *str_slot = (Slot){0};
            ++str_server->int_timeouts;
        }
    }
    mutex_unlock(&str_server->str_mutex);
    for (unsigned int int_i = 0; int_i < AST_PLAYERS; ++int_i) if (arr_expired[int_i]) {
        printf("SERVER_TIMEOUT identity=%u id=%u\n", arr_expired[int_i], int_i + 1);
        audit_event(&str_server->str_audit, arr_expired[int_i], "TIMEOUT", "lease");
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
    str_server.str_options = str_options;
    sim_init(&str_server.str_sim, UINT32_C(0xC5D3116));
    if (!audit_open(&str_server.str_audit, str_options.str_audit_file)) { fprintf(stderr, "audit_open failed\n"); return 1; }
    audit_event(&str_server.str_audit, 0, "START", "startup");
    if (atomic_load(&str_server.str_audit.b_failed)) { audit_close(&str_server.str_audit); return 1; }
    audit_dds_install(&str_server.str_audit);
    int int_result = pthread_create(&str_thread, NULL, server_worker, &str_server);
    if (int_result) {
        fprintf(stderr, "pthread_create: %s\n", strerror(int_result));
        audit_dds_uninstall(); audit_close(&str_server.str_audit); return 1;
    }
    while (!atomic_load(&str_server.b_ready) && !atomic_load(&str_server.b_stop) && !b_signal) sleep_ns(1000000);
    uint64_t int_start = monotonic_ns(), int_next = int_start;
    unsigned int int_ticks = str_options.int_ticks ? str_options.int_ticks : (unsigned int)(str_options.dbl_duration * AST_HZ);
    for (unsigned int int_tick = 0; int_tick < int_ticks && !b_signal && !atomic_load(&str_server.b_stop) &&
        !atomic_load(&str_server.str_audit.b_failed); ++int_tick) {
        server_expire(&str_server, monotonic_ns());
        game_step(&str_server);
        int_next += UINT64_C(1000000000) / AST_HZ;
        uint64_t int_now = monotonic_ns();
        if (int_next > int_now) sleep_ns(int_next - int_now);
    }
    atomic_store(&str_server.b_stop, true);
    int_result = pthread_join(str_thread, NULL);
    if (int_result) { fprintf(stderr, "pthread_join: %s\n", strerror(int_result)); return 1; }
    summary(&str_server);
    audit_dds_uninstall();
    bool b_audit_ok = audit_close(&str_server.str_audit);
    int_result = pthread_mutex_destroy(&str_server.str_mutex);
    return int_result || !b_audit_ok || atomic_load(&str_server.b_failed) ? 1 : 0;
}
