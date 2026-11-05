/* Live account denials from a native-certificate peer. Team + AI, 2026-11-05. */
#include <openssl/crypto.h>
#include <stdio.h>
#include <string.h>
#include "common/account_protocol.h"
#include "common/protocol.h"
#include "common/security.h"
#include "common/wire.h"

static bool rejected_join(dds_entity_t int_writer, dds_entity_t int_reader)
{
    Ast_Join str_join = {.int_version = AST_VERSION};
    if (!random_bytes(str_join.arr_nonce, sizeof(str_join.arr_nonce))) return false;
    uint64_t int_deadline = monotonic_ns() + UINT64_C(8000000000);
    while (monotonic_ns() < int_deadline) {
        ++str_join.int_seq;
        if (!dds_ok(wire_write_join(int_writer, &str_join), "probe_join_write")) return false;
        sleep_ns(AST_JOIN_NS);
        Ast_Response str_reply = {0};
        void *arr_samples[] = {&str_reply};
        dds_sample_info_t str_info;
        dds_return_t int_count = dds_take(int_reader, arr_samples, &str_info, 1, 1);
        if (!dds_ok(int_count, "probe_response_take")) return false;
        if (int_count && str_info.valid_data && response_valid(&str_reply) &&
            !memcmp(str_reply.arr_nonce, str_join.arr_nonce, sizeof(str_reply.arr_nonce)))
            return str_reply.int_status == 1;
    }
    return false;
}

static bool exchange(dds_entity_t int_writer, dds_entity_t int_reader, AstAccount_Request *str_req,
                     AstAccount_Response *str_rsp, uint16_t int_status)
{
    ++str_req->int_seq;
    if (!random_bytes(str_req->arr_request_nonce, sizeof(str_req->arr_request_nonce))) return false;
    uint64_t int_deadline = monotonic_ns() + UINT64_C(8000000000);
    while (monotonic_ns() < int_deadline) {
        if (!dds_ok(wire_write_account_request(int_writer, str_req), "probe_account_write")) return false;
        sleep_ns(UINT64_C(100000000));
        void *arr_samples[] = {str_rsp};
        dds_sample_info_t str_info;
        dds_return_t int_count = dds_take(int_reader, arr_samples, &str_info, 1, 1);
        if (!dds_ok(int_count, "probe_account_take")) return false;
        if (int_count && str_info.valid_data && account_response_valid(str_rsp) &&
            str_rsp->int_request_seq == str_req->int_seq &&
            str_rsp->int_operation == str_req->int_operation &&
            !memcmp(str_rsp->arr_request_nonce, str_req->arr_request_nonce, sizeof(str_req->arr_request_nonce))) {
            if (str_rsp->int_status != int_status)
                fprintf(stderr, "Probe operation=%u expected=%u received=%u\n",
                        (unsigned)str_req->int_operation, (unsigned)int_status, (unsigned)str_rsp->int_status);
            return str_rsp->int_status == int_status;
        }
    }
    fprintf(stderr, "Probe operation=%u timed out\n", (unsigned)str_req->int_operation);
    return false;
}

static bool new_challenge(dds_entity_t int_writer, dds_entity_t int_reader, AstAccount_Request *str_req)
{
    AstAccount_Request str_hello = {.int_version = AstAccount_VERSION, .int_operation = AstAccount_HELLO,
        .int_seq = str_req->int_seq};
    AstAccount_Response str_rsp = {0};
    if (!random_bytes(str_hello.arr_client_nonce, sizeof(str_hello.arr_client_nonce)) ||
        !exchange(int_writer, int_reader, &str_hello, &str_rsp, AstAccount_OK)) return false;
    str_req->int_seq = str_hello.int_seq;
    memcpy(str_req->arr_client_nonce, str_hello.arr_client_nonce, sizeof(str_req->arr_client_nonce));
    memcpy(str_req->arr_server_epoch, str_rsp.arr_server_epoch, sizeof(str_req->arr_server_epoch));
    memcpy(str_req->arr_server_challenge, str_rsp.arr_server_challenge, sizeof(str_req->arr_server_challenge));
    return true;
}

static bool rejected_account(dds_entity_t int_writer, dds_entity_t int_reader)
{
    AstAccount_Request str_req = {.int_version = AstAccount_VERSION, .int_operation = AstAccount_LEADERBOARD};
    AstAccount_Response str_rsp = {0};
    if (!new_challenge(int_writer, int_reader, &str_req) ||
        !random_bytes(str_req.arr_account_session, sizeof(str_req.arr_account_session)) ||
        !exchange(int_writer, int_reader, &str_req, &str_rsp, AstAccount_NOT_AUTHENTICATED)) return false;
    memset(str_req.arr_account_session, 0, sizeof(str_req.arr_account_session));
    str_req.int_operation = AstAccount_LOGIN;
    const char arr_name[] = "missing_account";
    str_req.int_username_length = sizeof(arr_name) - 1;
    memcpy(str_req.arr_username, arr_name, str_req.int_username_length);
    str_req.int_password_length = sizeof(str_req.arr_password);
    bool b_ok = random_bytes(str_req.arr_password, sizeof(str_req.arr_password));
    for (size_t int_i = 0; int_i < sizeof(str_req.arr_password); ++int_i)
        str_req.arr_password[int_i] = (uint8_t)('a' + str_req.arr_password[int_i] % 26u);
    b_ok = b_ok && exchange(int_writer, int_reader, &str_req, &str_rsp, AstAccount_INVALID_CREDENTIALS) &&
           new_challenge(int_writer, int_reader, &str_req) &&
           exchange(int_writer, int_reader, &str_req, &str_rsp, AstAccount_RATE_LIMITED);
    OPENSSL_cleanse(&str_req, sizeof(str_req));
    OPENSSL_cleanse(&str_rsp, sizeof(str_rsp));
    return b_ok;
}

int main(int int_argc, char **arr_argv)
{
    Options str_options;
    if (!options_parse(int_argc, arr_argv, &str_options, false)) return 2;
    client_trace_sink(true);
    dds_entity_t int_participant = secure_participant(&str_options, false);
    if (!dds_ok(int_participant, "probe_participant")) return 1;
    unsigned int int_identity = str_options.int_identity;
    dds_entity_t int_join = endpoint(int_participant, &Ast_Join_desc, "Join", int_identity, true, true);
    dds_entity_t int_reply = endpoint(int_participant, &Ast_Response_desc, "Response", int_identity, false, true);
    dds_entity_t int_forged = endpoint(int_participant, &AstAccount_Response_desc, "AccountResponse",
                                      int_identity, true, true);
    dds_entity_t int_account = endpoint(int_participant, &AstAccount_Request_desc, "AccountRequest",
                                       int_identity, true, true);
    dds_entity_t int_result = endpoint(int_participant, &AstAccount_Response_desc, "AccountResponse",
                                      int_identity, false, true);
    bool b_ok = int_join > 0 && int_reply > 0 && int_forged < 0 && rejected_join(int_join, int_reply);
    if (b_ok) puts("PASS certificate alone cannot join or publish an account response");
    b_ok = b_ok && int_account > 0 && int_result > 0 && rejected_account(int_account, int_result);
    b_ok = dds_ok(dds_delete(int_participant), "probe_delete") && b_ok;
    if (b_ok) puts("PASS forged account session, unknown account and rapid credential retry denied");
    return b_ok ? 0 : 1;
}
