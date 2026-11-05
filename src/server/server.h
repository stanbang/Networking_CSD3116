/**
 * @file
 * @brief Server simulation state, authenticated slots and reconnect lifecycle interfaces.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
#ifndef AST_SERVER_H
#define AST_SERVER_H
#include "common/protocol.h"
#include "common/audit_log.h"
#include "common/frame.h"
#include "server/accounts.h"
typedef struct {
    bool b_active, b_disconnected;
    uint64_t int_generation;
    unsigned int int_identity;
    uint8_t arr_nonce[16];
    uint64_t int_session, int_seq, int_seen;
    uint16_t int_keys;
    uint64_t int_client_time;
    uint64_t int_account_id;
} Slot;
typedef struct {
    uint64_t int_account_id, int_session;
    unsigned int int_identity;
} ScoreOwner;
typedef struct {
    pthread_mutex_t str_mutex;
    Options str_options;
    Audit str_audit;
    atomic_bool b_stop, b_failed, b_ready;
    Slot arr_slots[AST_PLAYERS];
    Sim str_sim;
    uint64_t arr_sim_session[AST_PLAYERS];
    ScoreOwner arr_score_owner[AST_PLAYERS];
    Accounts *ptr_accounts;
    uint8_t arr_boot_id[16];
    uint64_t int_round;
    bool b_started, b_paused, b_timeout_end;
    uint16_t int_reconnect_seconds;
    uint64_t int_round_end;
    uint8_t arr_used_nonce[AST_IDENTITIES][AST_JOIN_HISTORY][16];
    unsigned int arr_nonce_count[AST_IDENTITIES];
    uint64_t int_tick;
    uint64_t int_joins, int_rejects, int_leaves, int_timeouts, int_invalid, int_stale, int_inputs;
    unsigned int int_peak;
    uint64_t int_rate_hits;
    uint64_t int_snapshots;
    bool b_test_prepared;
} Server;
/** @brief Run the server DDS worker and publish authoritative game snapshots. */
void *server_worker(void *ptr_server);
/** @brief Check input leases and advance the reconnect timeout state. */
void server_expire(Server *str_server, uint64_t int_now);
/**
 * @brief Mark a player as disconnected and reserve the ship for reconnect.
 *
 * Caller holds the game mutex. The account worker never takes it.
 */
void server_release_slot(Server *str_server, unsigned int int_slot);
#endif
