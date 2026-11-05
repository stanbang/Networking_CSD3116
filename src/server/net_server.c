/* Session-bound admission on native-secured identity channels. Author: <team>, Codex assistance. Date: 2026-11-03. */
#include <stdio.h>
#include <stdlib.h>
#include <openssl/crypto.h>
#include "common/event_queue.h"
#include <string.h>
#include "server/server.h"
#include "common/security.h"
#include "common/wire.h"
typedef struct {
    dds_entity_t int_participant;
    EventQueue str_events;
    EventReader arr_events[AST_DDS_READERS];
    dds_entity_t arr_join[AST_IDENTITIES], arr_input[AST_IDENTITIES], arr_leave[AST_IDENTITIES];
    dds_entity_t arr_response[AST_IDENTITIES];
    dds_entity_t arr_account_request[AST_IDENTITIES], arr_account_response[AST_IDENTITIES];
    uint64_t int_response_seq;
    dds_entity_t int_snapshot;
    uint64_t int_snapshot_seq, int_last_snapshot;
    uint64_t arr_window[AST_IDENTITIES][3];
    unsigned int arr_count[AST_IDENTITIES][3];
} Network;

/* Called locked. Identity is supplied by the reader channel, never the message. */
static int find_identity(Server *str_server, unsigned int int_identity)
{
    for (int int_i = 0; int_i < AST_PLAYERS; ++int_i)
        if (str_server->arr_slots[int_i].b_active && str_server->arr_slots[int_i].int_identity == int_identity) return int_i;
    return -1;
}

/* Called locked. Retired nonces cannot reopen a slot; history exhaustion fails closed. */
static bool nonce_available(const Server *str_server, unsigned int int_identity, const uint8_t arr_nonce[16])
{
    unsigned int int_index = int_identity - 1;
    if (str_server->arr_nonce_count[int_index] == AST_JOIN_HISTORY) return false;
    for (unsigned int int_i = 0; int_i < str_server->arr_nonce_count[int_index]; ++int_i)
        if (memcmp(str_server->arr_used_nonce[int_index][int_i], arr_nonce, 16) == 0) return false;
    return true;
}

/* Called locked. At most five slots are searched; no random placement loop. */
static int allocate_slot(Server *str_server, unsigned int int_identity, const Ast_Join *str_join,
                         uint64_t int_session, uint64_t int_account_id)
{
    int int_slot = -1;
    unsigned int int_active = 0;
    for (int int_i = 0; int_i < AST_PLAYERS; ++int_i) {
        if (str_server->arr_slots[int_i].b_active) ++int_active;
        else if (int_slot == -1) int_slot = int_i;
    }
    if (int_slot < 0 || !accounts_game_joined(str_server->ptr_accounts, int_identity,
            str_join->arr_nonce, int_account_id)) return -1;
    Slot *str_slot = &str_server->arr_slots[int_slot];
    *str_slot = (Slot){.b_active = true, .int_identity = int_identity,
        .int_session = int_session, .int_seen = monotonic_ns(), .int_account_id = int_account_id};
    memcpy(str_slot->arr_nonce, str_join->arr_nonce, 16);
    unsigned int int_nonce_index = str_server->arr_nonce_count[int_identity - 1]++;
    memcpy(str_server->arr_used_nonce[int_identity - 1][int_nonce_index], str_join->arr_nonce, 16);
    ++str_server->int_joins;
    if (int_active + 1 > str_server->int_peak) str_server->int_peak = int_active + 1;
    return int_slot;
}

/* Idempotent active retries repeat the assignment; a new nonce cannot steal it. */
static void admit(Server *str_server, unsigned int int_identity, const Ast_Join *str_join,
                  uint64_t int_session, Ast_Response *str_response)
{
    mutex_lock(&str_server->str_mutex);
    uint64_t int_account_id = 0;
    int int_slot = accounts_authorize_join(str_server->ptr_accounts, int_identity, str_join->arr_nonce,
        &int_account_id) ? find_identity(str_server, int_identity) : -2;
    if (int_slot >= 0 && memcmp(str_server->arr_slots[int_slot].arr_nonce, str_join->arr_nonce, 16) != 0) int_slot = -2;
    if (int_slot == -1) int_slot = nonce_available(str_server, int_identity, str_join->arr_nonce) ?
        allocate_slot(str_server, int_identity, str_join, int_session, int_account_id) : -2;
    if (int_slot >= 0) {
        str_response->int_player_id = (uint16_t)(int_slot + 1);
        str_response->int_session = str_server->arr_slots[int_slot].int_session;
    } else { str_response->int_status = 1; ++str_server->int_rejects; }
    mutex_unlock(&str_server->str_mutex);
}

/* Randomness and DDS writes happen outside the shared-state mutex. */
static bool join_message(Server *str_server, Network *str_net, unsigned int int_channel, const Ast_Join *str_join)
{
    if (!join_valid(str_join)) {
        mutex_lock(&str_server->str_mutex); ++str_server->int_invalid; mutex_unlock(&str_server->str_mutex);
        printf("SERVER_DROP identity=%u reason=join-invalid\n", int_channel + 1);
        audit_event(&str_server->str_audit, int_channel + 1, "DROP", "join-invalid");
        return true;
    }
    uint64_t int_session = 0;
    if (!random_bytes(&int_session, sizeof(int_session)) || !int_session) return false;
    Ast_Response str_response = {.int_version = AST_VERSION, .int_seq = ++str_net->int_response_seq};
    memcpy(str_response.arr_nonce, str_join->arr_nonce, 16);
    admit(str_server, int_channel + 1, str_join, int_session, &str_response);
    audit_event(&str_server->str_audit, int_channel + 1, str_response.int_status ? "REJECT" : "JOIN", "admission");
    printf("SERVER_JOIN identity=%u id=%u status=%s\n", int_channel + 1, str_response.int_player_id,
           str_response.int_status ? "rejected" : "accepted");
    dds_return_t int_result = wire_write_response(str_net->arr_response[int_channel], &str_response);
    if (int_result == DDS_RETCODE_TIMEOUT) {
        mutex_lock(&str_server->str_mutex); ++str_server->int_rate_hits; mutex_unlock(&str_server->str_mutex);
        printf("SERVER_BACKPRESSURE identity=%u kind=response\n", int_channel + 1);
        audit_event(&str_server->str_audit, int_channel + 1, "BACKPRESSURE", "response");
        return true;
    }
    return dds_ok(int_result, "response_write");
}

/* Called locked after structural validation; returns a constant rejection reason. */
static const char *session_check(Server *str_server, unsigned int int_identity, unsigned int int_id,
                                 uint64_t int_session, uint64_t int_seq)
{
    int int_slot = find_identity(str_server, int_identity);
    if (int_slot < 0 || str_server->arr_slots[int_slot].int_session != int_session) return "session";
    if ((unsigned int)int_slot + 1 != int_id) return "spoof";
    if (int_seq <= str_server->arr_slots[int_slot].int_seq) return "replay";
    return NULL;
}

/* Network fields are validated before any index, and cannot extend a lease on failure. */
static void input_message(Server *str_server, unsigned int int_identity, const Ast_Input *str_input)
{
    const char *str_reason = NULL;
    if (!input_valid(str_input)) str_reason = str_input->int_player_id == 0 ? "id-zero" :
        str_input->int_player_id > AST_PLAYERS ? "id-high" : "input-invalid";
    mutex_lock(&str_server->str_mutex);
    if (!str_reason && str_input->int_tick_ack > str_server->int_tick) str_reason = "tick";
    if (!str_reason) str_reason = session_check(str_server, int_identity, str_input->int_player_id,
                                               str_input->int_session, str_input->int_seq);
    if (str_reason) {
        if (strcmp(str_reason, "replay") == 0) ++str_server->int_stale;
        else ++str_server->int_invalid;
    } else {
        Slot *str_slot = &str_server->arr_slots[str_input->int_player_id - 1];
        str_slot->int_seq = str_input->int_seq;
        str_slot->int_seen = monotonic_ns();
        str_slot->int_keys = str_input->int_keys;
        str_slot->int_client_time = str_input->int_client_time;
        ++str_server->int_inputs;
    }
    mutex_unlock(&str_server->str_mutex);
    if (str_reason) {
        printf("SERVER_DROP identity=%u reason=%s\n", int_identity, str_reason);
        audit_event(&str_server->str_audit, int_identity, "DROP", str_reason);
    }
}

/* Leave releases only the session bound to this channel. */
static void leave_message(Server *str_server, unsigned int int_identity, const Ast_Leave *str_leave)
{
    const char *str_reason = leave_valid(str_leave) ? NULL : "leave-invalid";
    mutex_lock(&str_server->str_mutex);
    if (!str_reason) str_reason = session_check(str_server, int_identity, str_leave->int_player_id,
                                               str_leave->int_session, str_leave->int_seq);
    if (str_reason) ++str_server->int_invalid;
    else { server_release_slot(str_server, str_leave->int_player_id - 1); ++str_server->int_leaves; }
    mutex_unlock(&str_server->str_mutex);
    if (str_reason) printf("SERVER_DROP identity=%u reason=%s\n", int_identity, str_reason);
    else printf("SERVER_LEAVE identity=%u id=%u\n", int_identity, str_leave->int_player_id);
    audit_event(&str_server->str_audit, int_identity, str_reason ? "DROP" : "LEAVE", str_reason ? str_reason : "explicit");
}

/* A validated account close releases only its own admitted game. */
static void account_release_game(Server *str_server, unsigned int int_identity, const uint8_t arr_nonce[16])
{
    mutex_lock(&str_server->str_mutex);
    int int_slot = find_identity(str_server, int_identity);
    bool b_released = int_slot >= 0 && memcmp(str_server->arr_slots[int_slot].arr_nonce, arr_nonce, 16) == 0;
    if (b_released) {
        server_release_slot(str_server, (unsigned int)int_slot);
        ++str_server->int_leaves;
    }
    mutex_unlock(&str_server->str_mutex);
    if (b_released) {
        printf("SERVER_LEAVE identity=%u id=%d\n", int_identity, int_slot + 1);
        audit_event(&str_server->str_audit, int_identity, "LEAVE", "explicit");
    }
}

static bool drain_account(Server *str_server, Network *str_net, unsigned int int_channel)
{
    dds_entity_t int_reader = str_net->arr_account_request[int_channel];
    void *arr_samples[AST_BATCH] = {0};
    dds_sample_info_t arr_info[AST_BATCH] = {0};
    dds_return_t int_count = dds_take(int_reader, arr_samples, arr_info, AST_BATCH, AST_BATCH);
    if (!dds_ok(int_count, "account_take")) return false;
    bool b_ok = true;
    for (dds_return_t int_i = 0; int_i < int_count && b_ok; ++int_i) {
        if (!arr_info[int_i].valid_data) continue;
        const AstAccount_Request *str_request = arr_samples[int_i];
        bool b_release = false;
        b_ok = accounts_receive(str_server->ptr_accounts, int_channel + 1, str_request, &b_release);
        if (b_ok && b_release) account_release_game(str_server, int_channel + 1, str_request->arr_client_nonce);
    }
    if (int_count && !dds_ok(dds_return_loan(int_reader, arr_samples, int_count), "account_return_loan")) b_ok = false;
    return b_ok && (int_count != AST_BATCH || event_queue_mark(&str_net->str_events, AST_GAME_READERS + int_channel));
}

static bool publish_accounts(Server *str_server, Network *str_net)
{
    for (unsigned int int_i = 0; int_i < 2; ++int_i) {
        AstAccount_Response str_response = {0};
        unsigned int int_identity = 0;
        if (!accounts_next_response(str_server->ptr_accounts, &int_identity, &str_response)) break;
        dds_return_t int_result = int_identity && int_identity <= AST_IDENTITIES ?
            wire_write_account_response(str_net->arr_account_response[int_identity - 1], &str_response) :
            DDS_RETCODE_BAD_PARAMETER;
        OPENSSL_cleanse(&str_response, sizeof(str_response));
        if (int_result != DDS_RETCODE_TIMEOUT && !dds_ok(int_result, "account_write")) return false;
    }
    return !accounts_failed(str_server->ptr_accounts);
}

/* Bound protected-reader work per identity; only one denial is logged per window. */
static bool rate_allow(Server *str_server, Network *str_net, unsigned int int_channel, unsigned int int_kind)
{
    unsigned int int_bucket = int_kind;
    unsigned int int_limit = int_bucket == 0 ? AST_JOIN_RATE : int_bucket == 1 ? AST_INPUT_RATE : AST_LEAVE_RATE;
    uint64_t int_now = monotonic_ns();
    if (int_now - str_net->arr_window[int_channel][int_bucket] >= UINT64_C(1000000000)) {
        str_net->arr_window[int_channel][int_bucket] = int_now;
        str_net->arr_count[int_channel][int_bucket] = 0;
    }
    unsigned int int_count = ++str_net->arr_count[int_channel][int_bucket];
    if (int_count <= int_limit) return true;
    mutex_lock(&str_server->str_mutex); ++str_server->int_rate_hits; mutex_unlock(&str_server->str_mutex);
    if (int_count == int_limit + 1) {
        const char *str_kind = int_bucket == 0 ? "join" : int_bucket == 1 ? "input" : "leave";
        printf("SERVER_RATE identity=%u kind=%s\n", int_channel + 1, str_kind);
        audit_event(&str_server->str_audit, int_channel + 1, "RATE", str_kind);
    }
    return false;
}

/* At most one bounded loan per ready reader; every loan is returned on all paths. */
static bool drain(Server *str_server, Network *str_net, unsigned int int_tag)
{
    unsigned int int_channel = int_tag / 3, int_kind = int_tag % 3;
    dds_entity_t int_reader = int_kind == 0 ? str_net->arr_join[int_channel] :
        int_kind == 1 ? str_net->arr_input[int_channel] : str_net->arr_leave[int_channel];
    void *arr_samples[AST_BATCH] = {0};
    dds_sample_info_t arr_info[AST_BATCH] = {0};
    dds_return_t int_count = dds_take(int_reader, arr_samples, arr_info, AST_BATCH, AST_BATCH);
    if (!dds_ok(int_count, "take")) return false;
    bool b_ok = true;
    for (dds_return_t int_i = 0; int_i < int_count && b_ok; ++int_i) if (arr_info[int_i].valid_data) {
        if (atomic_load(&str_server->str_audit.b_failed)) { b_ok = false; break; }
        if (!rate_allow(str_server, str_net, int_channel, int_kind)) continue;
        if (int_kind == 0) b_ok = join_message(str_server, str_net, int_channel, arr_samples[int_i]);
        else if (int_kind == 1) input_message(str_server, int_channel + 1, arr_samples[int_i]);
        else leave_message(str_server, int_channel + 1, arr_samples[int_i]);
    }
    if (int_count > 0 && !dds_ok(dds_return_loan(int_reader, arr_samples, int_count), "return_loan")) b_ok = false;
    if (b_ok && int_count == AST_BATCH) b_ok = event_queue_mark(&str_net->str_events, int_tag);
    return b_ok;
}

/* Fixed channel count keeps admission memory and per-wake work bounded. */
static bool network_init(Server *str_server, Network *str_net)
{
    str_net->int_participant = secure_participant(&str_server->str_options, true);
    if (!dds_ok(str_net->int_participant, "create_participant")) return false;
    str_net->int_snapshot = endpoint(str_net->int_participant, &Ast_Snapshot_desc, "Snapshot", 0, true, false);
    if (str_net->int_snapshot < 0) return false;
    for (unsigned int int_i = 0; int_i < AST_IDENTITIES; ++int_i) {
        str_net->arr_join[int_i] = endpoint(str_net->int_participant, &Ast_Join_desc, "Join", int_i + 1, false, true);
        str_net->arr_input[int_i] = endpoint(str_net->int_participant, &Ast_Input_desc, "Input", int_i + 1, false, false);
        str_net->arr_leave[int_i] = endpoint(str_net->int_participant, &Ast_Leave_desc, "Leave", int_i + 1, false, true);
        str_net->arr_response[int_i] = endpoint(str_net->int_participant, &Ast_Response_desc, "Response", int_i + 1, true, true);
        str_net->arr_account_request[int_i] = endpoint(str_net->int_participant, &AstAccount_Request_desc,
            "AccountRequest", int_i + 1, false, true);
        str_net->arr_account_response[int_i] = endpoint(str_net->int_participant, &AstAccount_Response_desc,
            "AccountResponse", int_i + 1, true, true);
        if (str_net->arr_join[int_i] < 0 || str_net->arr_input[int_i] < 0 || str_net->arr_leave[int_i] < 0 ||
            str_net->arr_response[int_i] < 0 || str_net->arr_account_request[int_i] < 0 ||
            str_net->arr_account_response[int_i] < 0) return false;
        if (!event_reader_attach(&str_net->arr_events[int_i * 3], str_net->arr_join[int_i]) ||
            !event_reader_attach(&str_net->arr_events[int_i * 3 + 1], str_net->arr_input[int_i]) ||
            !event_reader_attach(&str_net->arr_events[int_i * 3 + 2], str_net->arr_leave[int_i]) ||
            !event_reader_attach(&str_net->arr_events[AST_GAME_READERS + int_i], str_net->arr_account_request[int_i]))
            return false;
    }
    /* One seed covers samples received before the listeners were installed. */
    for (unsigned int int_i = 0; int_i < AST_DDS_READERS; ++int_i)
        if (!event_queue_mark(&str_net->str_events, int_i)) return false;
    return true;
}

/* Copy under lock, serialize/write outside; all bounded arrays belong to this stack. */
static bool publish_snapshot(Server *str_server, Network *str_net)
{
    uint64_t int_now = monotonic_ns();
    if (int_now - str_net->int_last_snapshot < AST_SNAPSHOT_NS) return true;
    str_net->int_last_snapshot = int_now;
    Frame str_frame = {0}; Ast_Snapshot str_snapshot = {0};
    mutex_lock(&str_server->str_mutex);
    frame_from_sim(&str_frame, &str_server->str_sim);
    for (unsigned int int_i = 0; int_i < str_frame.int_ships; ++int_i) {
        unsigned int int_slot = str_frame.arr_ships[int_i].int_player_id - 1;
        str_frame.arr_ships[int_i].int_client_time = str_server->arr_slots[int_slot].int_client_time;
    }
    ++str_server->int_snapshots;
    mutex_unlock(&str_server->str_mutex);
    str_frame.int_seq = ++str_net->int_snapshot_seq;
    frame_to_wire(&str_frame, &str_snapshot);
    return dds_ok(wire_write_snapshot(str_net->int_snapshot, &str_snapshot), "snapshot_write");
}

/* Sole DDS I/O owner. Listener bits and monotonic condition wait avoid polling. */
void *server_worker(void *ptr_server)
{
    Server *str_server = ptr_server;
    Network str_net = {0};
    bool b_ok = event_queue_init(&str_net.str_events);
    for (unsigned int int_i = 0; b_ok && int_i < AST_DDS_READERS; ++int_i)
        b_ok = event_reader_init(&str_net.arr_events[int_i], &str_net.str_events, int_i);
    if (b_ok) b_ok = network_init(str_server, &str_net);
    if (b_ok) accounts_set_events(str_server->ptr_accounts, &str_net.str_events);
    if (b_ok) { puts("SERVER_READY security=NATIVE_DDS domain=42"); atomic_store(&str_server->b_ready, true); }
    while (b_ok && !atomic_load(&str_server->b_stop)) {
        uint32_t int_ready = 0;
        if (!event_queue_wait(&str_net.str_events, &int_ready)) { b_ok = false; break; }
        /* Queue lock is released before DDS loans, crypto, audit or game locks. */
        for (unsigned int int_i = 0; int_i < AST_DDS_READERS && b_ok; ++int_i) {
            if (!(int_ready & (UINT32_C(1) << int_i))) continue;
            b_ok = int_i < AST_GAME_READERS ? drain(str_server, &str_net, int_i) :
                drain_account(str_server, &str_net, int_i - AST_GAME_READERS);
        }
        if (b_ok) b_ok = publish_snapshot(str_server, &str_net) && publish_accounts(str_server, &str_net);
    }
    /* Native listener removal waits active callbacks; never hold the queue lock here. */
    accounts_set_events(str_server->ptr_accounts, NULL);
    for (unsigned int int_i = 0; int_i < AST_DDS_READERS; ++int_i)
        if (!event_reader_detach(&str_net.arr_events[int_i])) b_ok = false;
    if (str_net.int_participant > 0 && !dds_ok(dds_delete(str_net.int_participant), "delete_participant")) {
        fputs("DDS deletion failed; cannot release callback context safely\n", stderr); _Exit(EXIT_FAILURE);
    }
    if (str_net.str_events.b_initialized && !event_queue_fini(&str_net.str_events)) {
        fputs("Event queue teardown failed\n", stderr); _Exit(EXIT_FAILURE);
    }
    if (!b_ok) atomic_store(&str_server->b_failed, true);
    atomic_store(&str_server->b_stop, true);
    return NULL;
}
