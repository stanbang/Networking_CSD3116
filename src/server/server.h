/* P1 lease/admission state, no game sim yet. Author: <team>, Codex assistance. Date: 2026-11-03. */
#ifndef AST_SERVER_H
#define AST_SERVER_H
#include "common/protocol.h"
#include "common/audit_log.h"
#include "common/frame.h"
typedef struct {
    bool b_active;
    unsigned int int_identity;
    uint8_t arr_nonce[16];
    uint64_t int_session, int_seq, int_seen;
    uint16_t int_keys;
    uint64_t int_client_time;
} Slot;
typedef struct {
    pthread_mutex_t str_mutex;
    Options str_options;
    Audit str_audit;
    atomic_bool b_stop, b_failed, b_ready;
    Slot arr_slots[AST_PLAYERS];
    Sim str_sim;
    uint64_t arr_sim_session[AST_PLAYERS];
    uint8_t arr_used_nonce[AST_IDENTITIES][AST_JOIN_HISTORY][16];
    unsigned int arr_nonce_count[AST_IDENTITIES];
    uint64_t int_tick;
    uint64_t int_joins, int_rejects, int_leaves, int_timeouts, int_invalid, int_stale, int_inputs;
    unsigned int int_peak;
    uint64_t int_rate_hits;
    uint64_t int_snapshots;
    bool b_test_prepared;
} Server;
void *server_worker(void *ptr_server);
void server_expire(Server *str_server, uint64_t int_now);
#endif
