/* Session-bound admission on native-secured identity channels. Author: <team>, Codex assistance. Date: 2026-11-03. */
#include <stdio.h>
#include <string.h>
#include "server/server.h"
#include "common/security.h"
typedef struct {
    dds_entity_t int_participant, int_waitset;
    dds_entity_t arr_join[AST_IDENTITIES], arr_input[AST_IDENTITIES], arr_leave[AST_IDENTITIES];
    dds_entity_t arr_response[AST_IDENTITIES];
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
static int allocate_slot(Server *str_server, unsigned int int_identity, const Ast_Join *str_join, uint64_t int_session)
{
    int int_slot = -1;
    unsigned int int_active = 0;
    for (int int_i = 0; int_i < AST_PLAYERS; ++int_i) {
        if (str_server->arr_slots[int_i].b_active) ++int_active;
        else if (int_slot == -1) int_slot = int_i;
    }
    if (int_slot < 0) return -1;
    Slot *str_slot = &str_server->arr_slots[int_slot];
    *str_slot = (Slot){.b_active = true, .int_identity = int_identity,
        .int_session = int_session, .int_seen = monotonic_ns()};
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
    int int_slot = find_identity(str_server, int_identity);
    if (int_slot >= 0 && memcmp(str_server->arr_slots[int_slot].arr_nonce, str_join->arr_nonce, 16) != 0) int_slot = -2;
    if (int_slot == -1) int_slot = nonce_available(str_server, int_identity, str_join->arr_nonce) ?
        allocate_slot(str_server, int_identity, str_join, int_session) : -2;
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
    dds_return_t int_result = dds_write(str_net->arr_response[int_channel], &str_response);
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
    else { str_server->arr_slots[str_leave->int_player_id - 1] = (Slot){0}; ++str_server->int_leaves; }
    mutex_unlock(&str_server->str_mutex);
    if (str_reason) printf("SERVER_DROP identity=%u reason=%s\n", int_identity, str_reason);
    else printf("SERVER_LEAVE identity=%u id=%u\n", int_identity, str_leave->int_player_id);
    audit_event(&str_server->str_audit, int_identity, str_reason ? "DROP" : "LEAVE", str_reason ? str_reason : "explicit");
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
    return b_ok;
}

/* Fixed channel count keeps admission memory and per-wake work bounded. */
static bool network_init(Server *str_server, Network *str_net)
{
    str_net->int_participant = secure_participant(&str_server->str_options, true);
    if (!dds_ok(str_net->int_participant, "create_participant")) return false;
    str_net->int_waitset = dds_create_waitset(str_net->int_participant);
    if (!dds_ok(str_net->int_waitset, "create_waitset")) return false;
    str_net->int_snapshot = endpoint(str_net->int_participant, &Ast_Snapshot_desc, "Snapshot", 0, true, false);
    if (str_net->int_snapshot < 0) return false;
    for (unsigned int int_i = 0; int_i < AST_IDENTITIES; ++int_i) {
        str_net->arr_join[int_i] = endpoint(str_net->int_participant, &Ast_Join_desc, "Join", int_i + 1, false, true);
        str_net->arr_input[int_i] = endpoint(str_net->int_participant, &Ast_Input_desc, "Input", int_i + 1, false, false);
        str_net->arr_leave[int_i] = endpoint(str_net->int_participant, &Ast_Leave_desc, "Leave", int_i + 1, false, true);
        str_net->arr_response[int_i] = endpoint(str_net->int_participant, &Ast_Response_desc, "Response", int_i + 1, true, true);
        if (str_net->arr_join[int_i] < 0 || str_net->arr_input[int_i] < 0 || str_net->arr_leave[int_i] < 0 ||
            str_net->arr_response[int_i] < 0) return false;
        if (!attach_reader(str_net->int_participant, str_net->int_waitset, str_net->arr_join[int_i], int_i * 3) ||
            !attach_reader(str_net->int_participant, str_net->int_waitset, str_net->arr_input[int_i], int_i * 3 + 1) ||
            !attach_reader(str_net->int_participant, str_net->int_waitset, str_net->arr_leave[int_i], int_i * 3 + 2)) return false;
    }
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
    return dds_ok(dds_write(str_net->int_snapshot, &str_snapshot), "snapshot_write");
}

/* Sole DDS owner. Waitsets replace baseline busy polling, with bounded shutdown wait. */
void *server_worker(void *ptr_server)
{
    Server *str_server = ptr_server;
    Network str_net = {0};
    bool b_ok = network_init(str_server, &str_net);
    if (b_ok) { puts("SERVER_READY security=NATIVE_DDS domain=42"); atomic_store(&str_server->b_ready, true); }
    while (b_ok && !atomic_load(&str_server->b_stop)) {
        dds_attach_t arr_tags[AST_IDENTITIES * 3] = {0};
        dds_return_t int_count = dds_waitset_wait(str_net.int_waitset, arr_tags, AST_IDENTITIES * 3, DDS_MSECS(50));
        if (!dds_ok(int_count, "waitset_wait")) { b_ok = false; break; }
        for (dds_return_t int_i = 0; int_i < int_count && b_ok; ++int_i) {
            if (arr_tags[int_i] < 0 || arr_tags[int_i] >= AST_IDENTITIES * 3) { b_ok = false; break; }
            b_ok = drain(str_server, &str_net, (unsigned int)arr_tags[int_i]);
        }
        if (b_ok) b_ok = publish_snapshot(str_server, &str_net);
    }
    if (str_net.int_participant > 0 && !dds_ok(dds_delete(str_net.int_participant), "delete_participant")) b_ok = false;
    atomic_store(&str_server->b_failed, !b_ok);
    atomic_store(&str_server->b_stop, true);
    return NULL;
}
