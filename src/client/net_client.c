/* Nonce-addressed finite networking on native-secured identity channels. Author: <team>, Codex assistance. Date: 2026-11-03. */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "common/event_queue.h"
#include <string.h>
#include "client/client.h"
#include "client/account_client.h"
#include "common/security.h"
#include "common/wire.h"
enum { RESPONSE_READER, SNAPSHOT_READER, ACCOUNT_READER, CLIENT_READERS };
typedef struct {
    dds_entity_t int_participant, int_join, int_response, int_input, int_leave;
    EventReader arr_events[CLIENT_READERS];
    dds_entity_t int_snapshot, int_account_request, int_account_response;
    ClientAccount str_account;
    uint64_t int_seq, int_session, int_last_send, int_response_seq;
    unsigned int int_player_id;
    Ast_Input str_previous;
    bool b_previous;
    uint64_t int_snapshot_seq, int_log_time;
    bool b_gameover;
} Network;

/* Print public addressing nonces for test correlation, never session tokens. */
static void join_log(const Network *str_net, bool b_rejected)
{
    char str_nonce[33] = {0};
    const char str_hex[] = "0123456789abcdef";
    for (unsigned int int_i = 0; int_i < 16; ++int_i) {
        str_nonce[int_i * 2] = str_hex[str_net->str_account.arr_nonce[int_i] >> 4];
        str_nonce[int_i * 2 + 1] = str_hex[str_net->str_account.arr_nonce[int_i] & 15];
    }
    printf("CLIENT_JOIN nonce=%s id=%u status=%s\n", str_nonce, str_net->int_player_id,
           b_rejected ? "rejected" : "accepted");
}

/* Accept only the echoed nonce, known status, and valid session/id combination. */
static void response(Client *str_client, Network *str_net, const Ast_Response *str_response)
{
    if (!client_account_play(&str_net->str_account, str_client) || !response_valid(str_response) ||
        str_response->int_seq <= str_net->int_response_seq ||
        memcmp(str_response->arr_nonce, str_net->str_account.arr_nonce, 16) != 0) return;
    bool b_rejected = str_response->int_status == 1;
    str_net->int_response_seq = str_response->int_seq;
    if (str_net->int_player_id) return;
    str_net->int_player_id = str_response->int_player_id;
    str_net->int_session = str_response->int_session;
    atomic_store(&str_client->int_player_id, str_net->int_player_id);
    atomic_store(&str_client->b_rejected, b_rejected);
    if (!b_rejected) atomic_store(&str_client->b_joined_once, true);
    join_log(str_net, b_rejected);
    if (b_rejected) atomic_store(&str_client->b_stop, true);
}

/* Process a bounded reliable response loan, returning it even if all samples invalid. */
static void receive_snapshot(Client *str_client, Network *str_net, const Ast_Snapshot *str_snapshot)
{
    if (!snapshot_valid(str_snapshot) || str_snapshot->int_seq <= str_net->int_snapshot_seq) return;
    str_net->int_snapshot_seq = str_snapshot->int_seq;
    if (!str_net->int_player_id) return;
    if(str_net->b_gameover!=str_snapshot->b_gameover) {
        printf("CLIENT_GAME_STATE tick=%" PRIu64 " gameover=%u wave=%u\n",
            str_snapshot->int_tick,(unsigned int)str_snapshot->b_gameover,str_snapshot->int_wave);
        str_net->b_gameover=str_snapshot->b_gameover;
    }
    if (str_snapshot->b_timeout_end && !atomic_load(&str_client->int_end_deadline))
        atomic_store(&str_client->int_end_deadline, monotonic_ns() + AST_END_DISPLAY_NS);
    Frame str_frame = {0}; frame_from_wire(&str_frame,str_snapshot);
    mutex_lock(&str_client->str_mutex);
    str_client->arr_frames[str_client->int_head] = str_frame;
    str_client->int_head = (str_client->int_head + 1) % AST_FRAME_HISTORY;
    if (str_client->int_frames < AST_FRAME_HISTORY) ++str_client->int_frames;
    mutex_unlock(&str_client->str_mutex);
    atomic_store(&str_client->int_latest_tick,str_snapshot->int_tick);
    for (unsigned int int_i = 0; int_i < str_frame.int_ships; ++int_i) {
        const Ast_Ship *str_ship = &str_frame.arr_ships[int_i];
        if (str_ship->int_player_id != str_net->int_player_id) continue;
        uint64_t int_now = monotonic_ns();
        if (str_ship->int_client_time && str_ship->int_client_time <= int_now && int_now-str_ship->int_client_time < UINT64_C(60000000000))
            atomic_store(&str_client->int_ping_ms,(unsigned int)((int_now-str_ship->int_client_time)/1000000));
        if (int_now-str_net->int_log_time >= UINT64_C(1000000000)) {
            printf("CLIENT_SNAPSHOT tick=%" PRIu64 " id=%u x=%d y=%d score=%u lives=%u vx=%d vy=%d distance_mm=%" PRIu64 "\n",
                str_frame.int_tick,str_ship->int_player_id,str_ship->int_x,str_ship->int_y,str_ship->int_score,
                str_ship->int_lives,str_ship->int_vx,str_ship->int_vy,str_ship->int_distance_mm);
            str_net->int_log_time = int_now;
        }
    }
}

/* A reader loan never crosses the client mutex or returns to the render thread. */
static bool drain(Client *str_client, Network *str_net, unsigned int int_tag)
{
    const dds_entity_t arr_readers[] = {str_net->int_response, str_net->int_snapshot, str_net->int_account_response};
    dds_entity_t int_reader = arr_readers[int_tag];
    void *arr_samples[AST_BATCH] = {0};
    dds_sample_info_t arr_info[AST_BATCH] = {0};
    dds_return_t int_count = dds_take(int_reader, arr_samples, arr_info, AST_BATCH, AST_BATCH);
    if (!dds_ok(int_count, "response_take")) return false;
    bool b_ok = true;
    for (dds_return_t int_i = 0; b_ok && int_i < int_count; ++int_i) if (arr_info[int_i].valid_data) {
        if (int_tag == SNAPSHOT_READER) receive_snapshot(str_client, str_net, arr_samples[int_i]);
        else if (int_tag == ACCOUNT_READER) b_ok = client_account_receive(&str_net->str_account, str_client, arr_samples[int_i]);
        else response(str_client, str_net, arr_samples[int_i]);
    }
    if (int_count && !dds_ok(dds_return_loan(int_reader, arr_samples, int_count), "response_return_loan")) b_ok = false;
    if (b_ok && int_count == AST_BATCH) b_ok = event_queue_mark(&str_client->str_events, int_tag);
    return b_ok;
}

/* Test-only mutations preserve the real session locally so graceful leave still works. */
static void inject_fault(Client *str_client, Network *str_net, Ast_Input *str_input)
{
    const char *str_fault = str_client->str_options.str_fault;
    if (strcmp(str_fault, "spoof") == 0) str_input->int_player_id = (uint16_t)(str_net->int_player_id % AST_PLAYERS + 1);
    else if (strcmp(str_fault, "session") == 0) str_input->int_session ^= UINT64_C(0x8000000000000000);
    else if (strcmp(str_fault, "id-zero") == 0) str_input->int_player_id = 0;
    else if (strcmp(str_fault, "id-high") == 0) str_input->int_player_id = UINT16_MAX;
    else if (strcmp(str_fault, "replay") == 0 && str_net->b_previous) *str_input = str_net->str_previous;
}

/* Fault generation still uses native security; rate tests are authenticated traffic. */
static bool send_flood(Client *str_client, Network *str_net, Ast_Input *str_input)
{
    bool b_join = strcmp(str_client->str_options.str_fault, "join-flood") == 0;
    bool b_leave = strcmp(str_client->str_options.str_fault, "leave-flood") == 0;
    unsigned int int_count = b_join || b_leave || strcmp(str_client->str_options.str_fault, "flood") == 0 ? 100 : 1;
    for (unsigned int int_i = 0; int_i < int_count; ++int_i) {
        if (b_leave) {
            Ast_Leave str_leave={.int_version=AST_VERSION,.int_seq=++str_net->int_seq,
                .int_player_id=(uint16_t)str_net->int_player_id,.int_session=str_net->int_session ^ UINT64_C(0x8000000000000000)};
            if(!dds_ok(wire_write_leave(str_net->int_leave,&str_leave),"leave_flood_write")) return false;
        } else if (b_join) {
            Ast_Join str_join = {.int_version = AST_VERSION, .int_seq = ++str_net->int_seq};
            memcpy(str_join.arr_nonce, str_net->str_account.arr_nonce, 16);
            if (!dds_ok(wire_write_join(str_net->int_join, &str_join), "join_flood_write")) return false;
        } else {
            if (int_i) str_input->int_seq = ++str_net->int_seq;
            if (!dds_ok(wire_write_input(str_net->int_input, str_input), "input_write")) return false;
        }
    }
    if (b_join || b_leave) {
        str_input->int_seq = ++str_net->int_seq;
        return dds_ok(wire_write_input(str_net->int_input, str_input), "heartbeat_write");
    }
    return true;
}

/* A heartbeat carries input state and extends only a validated server lease. */
static bool send_message(Client *str_client, Network *str_net, uint64_t int_now)
{
    if (!str_net->int_player_id) {
        bool b_slow = strcmp(str_client->str_options.str_fault, "slow-reader") == 0;
        if (int_now - str_net->int_last_send < (b_slow ? AST_HEARTBEAT_NS : AST_JOIN_NS)) return true;
        Ast_Join str_join = {.int_version = AST_VERSION, .int_seq = ++str_net->int_seq};
        memcpy(str_join.arr_nonce, str_net->str_account.arr_nonce, 16);
        str_net->int_last_send = int_now;
        return dds_ok(wire_write_join(str_net->int_join, &str_join), "join_write");
    }
    if (int_now - str_net->int_last_send < AST_HEARTBEAT_NS) return true;
    str_net->int_last_send = int_now;
    /* Exercise response backpressure without letting the fault client's lease lapse. */
    if (strcmp(str_client->str_options.str_fault, "slow-reader") == 0) {
        Ast_Join str_join = {.int_version = AST_VERSION, .int_seq = ++str_net->int_seq};
        memcpy(str_join.arr_nonce, str_net->str_account.arr_nonce, 16);
        if (!dds_ok(wire_write_join(str_net->int_join, &str_join), "slow_join_write")) return false;
    }
    Ast_Input str_input = {.int_version = AST_VERSION, .int_seq = ++str_net->int_seq,
        .int_session = str_net->int_session, .int_player_id = (uint16_t)str_net->int_player_id,
        .int_tick_ack = atomic_load(&str_client->int_latest_tick), .int_client_time = int_now,
        .int_keys = (uint16_t)atomic_load(&str_client->int_keys)};
    inject_fault(str_client, str_net, &str_input);
    str_net->str_previous = str_input;
    str_net->b_previous = true;
    return send_flood(str_client, str_net, &str_input);
}

/* Creation errors stop the worker, and participant deletion rolls back all children. */
static bool network_init(Client *str_client, Network *str_net)
{
    str_net->int_participant = secure_participant(&str_client->str_options, false);
    if (!dds_ok(str_net->int_participant, "create_participant")) return false;
    unsigned int int_id = str_client->str_options.int_identity;
    if (strcmp(str_client->str_options.str_fault, "rogue-server") == 0) {
        dds_entity_t int_forbidden = endpoint(str_net->int_participant, &Ast_Response_desc, "Response", int_id, true, true);
        fprintf(stderr, "CLIENT_FORBIDDEN_WRITER result=%s\n", int_forbidden < 0 ? "denied" : "UNEXPECTED_ALLOWED");
        return false;
    }
    str_net->int_join = endpoint(str_net->int_participant, &Ast_Join_desc, "Join", int_id, true, true);
    str_net->int_input = endpoint(str_net->int_participant, &Ast_Input_desc, "Input", int_id, true, false);
    str_net->int_leave = endpoint(str_net->int_participant, &Ast_Leave_desc, "Leave", int_id, true, true);
    str_net->int_response = endpoint(str_net->int_participant, &Ast_Response_desc, "Response", int_id, false, true);
    str_net->int_snapshot = endpoint(str_net->int_participant, &Ast_Snapshot_desc, "Snapshot", 0, false, false);
    str_net->int_account_request = endpoint(str_net->int_participant, &AstAccount_Request_desc, "AccountRequest", int_id, true, true);
    str_net->int_account_response = endpoint(str_net->int_participant, &AstAccount_Response_desc, "AccountResponse", int_id, false, true);
    if (str_net->int_join < 0 || str_net->int_input < 0 || str_net->int_leave < 0 || str_net->int_response < 0 ||
        str_net->int_snapshot < 0 || str_net->int_account_request < 0 || str_net->int_account_response < 0) return false;
    const dds_entity_t arr_readers[] = {str_net->int_response, str_net->int_snapshot, str_net->int_account_response};
    for (unsigned int int_i = 0; int_i < CLIENT_READERS; ++int_i)
        if (!event_reader_attach(&str_net->arr_events[int_i], arr_readers[int_i]) ||
            !event_queue_mark(&str_client->str_events, int_i)) return false;
    client_account_init(&str_net->str_account, str_client);
    return true;
}

/* Await transport ACK outside locks. The approved timeout fallback does not
 * prove server receipt; lifecycle tests independently verify slot release. */
static bool send_leave(Network *str_net)
{
    if (!str_net->int_player_id) return true;
    /* Counts reveal lost/extra matches without exposing reader handles or identities.
     * They are observations, not proof that the server processed this Leave. */
    dds_return_t int_before = dds_get_matched_subscriptions(str_net->int_leave, NULL, 0);
    bool b_before = dds_ok(int_before, "leave_matched_before");
    Ast_Leave str_leave = {.int_version = AST_VERSION, .int_seq = ++str_net->int_seq,
        .int_session = str_net->int_session, .int_player_id = (uint16_t)str_net->int_player_id};
    if (!dds_ok(wire_write_leave(str_net->int_leave, &str_leave), "leave_write")) return false;
    uint64_t int_started = monotonic_ns();
    dds_return_t int_ack = dds_wait_for_acks(str_net->int_leave, DDS_SECS(1));
    uint64_t int_finished = monotonic_ns();
    dds_return_t int_after = dds_get_matched_subscriptions(str_net->int_leave, NULL, 0);
    bool b_ack = int_ack == DDS_RETCODE_TIMEOUT || dds_ok(int_ack, "leave_ack");
    bool b_after = dds_ok(int_after, "leave_matched_after");
    bool b_clock = int_finished >= int_started;
    fprintf(stderr, "CLIENT_LEAVE_ACK matched_before=%" PRId32 " matched_after=%" PRId32
        " result=%" PRId32 " requested_ns=1000000000 elapsed_ns=%" PRIu64 " clock_valid=%u\n",
        (int32_t)int_before, (int32_t)int_after, (int32_t)int_ack,
        b_clock ? int_finished - int_started : UINT64_C(0), (unsigned int)b_clock);
    if (int_ack == DDS_RETCODE_TIMEOUT) {
        fputs("CLIENT_WARNING leave_ack_timeout: transport acknowledgement unconfirmed; "
              "server slot release requires independent verification\n", stderr);
    }
    return b_before && b_ack && b_after && b_clock;
}

static void reset_game(Client *str_client, Network *str_net)
{
    str_net->str_account.b_reset_game = false;
    str_net->int_player_id = 0;
    str_net->int_session = str_net->int_last_send = str_net->int_log_time = 0;
    str_net->b_previous = str_net->b_gameover = false;
    memset(&str_net->str_previous, 0, sizeof(str_net->str_previous));
    atomic_store(&str_client->int_player_id, 0);
    atomic_store(&str_client->int_keys, 0);
    atomic_store(&str_client->int_latest_tick, 0);
    atomic_store(&str_client->int_ping_ms, 0);
    mutex_lock(&str_client->str_mutex);
    str_client->int_head = str_client->int_frames = 0;
    mutex_unlock(&str_client->str_mutex);
}

/* Sole DDS I/O owner: bounded monotonic condition wait, notified by native listeners/UI. */
void *client_worker(void *ptr_client)
{
    Client *str_client = ptr_client;
    Network str_net = {.str_account.str_profile = {.int_directory = -1, .int_lock = -1}};
    client_trace_sink(true);
    bool b_ok = true;
    for (unsigned int int_i = 0; b_ok && int_i < CLIENT_READERS; ++int_i)
        b_ok = event_reader_init(&str_net.arr_events[int_i], &str_client->str_events, int_i);
    if (b_ok) b_ok = network_init(str_client, &str_net);
    atomic_store(&str_client->b_ready, b_ok);
    while (b_ok && !atomic_load(&str_client->b_stop)) {
        uint32_t int_ready = 0;
        b_ok = event_queue_wait(&str_client->str_events, &int_ready);
        for (unsigned int int_i = 0; b_ok && int_i < CLIENT_READERS; ++int_i) if (int_ready & (UINT32_C(1) << int_i)) {
            if (int_i == RESPONSE_READER && str_net.int_player_id && strcmp(str_client->str_options.str_fault, "slow-reader") == 0) continue;
            b_ok = drain(str_client, &str_net, int_i);
        }
        if (b_ok && !atomic_load(&str_client->b_stop))
            b_ok = client_account_tick(&str_net.str_account, str_client, str_net.int_account_request, str_net.int_player_id != 0);
        if (str_net.str_account.b_reset_game) reset_game(str_client, &str_net);
        if (b_ok && !atomic_load(&str_client->b_stop) && client_account_play(&str_net.str_account, str_client))
            b_ok = send_message(str_client, &str_net, monotonic_ns());
    }
    if (str_net.int_player_id) {
        if (b_ok) b_ok = send_leave(&str_net);
    } else if (!client_account_close(&str_net.str_account, str_net.int_account_request)) b_ok = false;
    for (unsigned int int_i = 0; int_i < CLIENT_READERS; ++int_i)
        if (!event_reader_detach(&str_net.arr_events[int_i])) b_ok = false;
    if (str_net.int_participant > 0 && !dds_ok(dds_delete(str_net.int_participant), "delete_participant")) {
        fputs("DDS deletion failed; cannot release callback context safely\n", stderr); _Exit(EXIT_FAILURE);
    }
    client_account_fini(&str_net.str_account);
    client_trace_sink(false);
    atomic_store(&str_client->b_failed, !b_ok);
    atomic_store(&str_client->b_stop, true);
    return NULL;
}
