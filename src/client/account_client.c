/* Client account RPCs. Passwords and bearer values never enter UI/log output. Team/Codex, 2026-11-05. */
#include <inttypes.h>
#include <openssl/crypto.h>
#include <stdio.h>
#include <string.h>
#include "client/account_client.h"
#include "common/account_protocol.h"
#include "common/wire.h"

#define ACCOUNT_RPC_NS UINT64_C(10000000000)
#define ACCOUNT_KEEPALIVE_NS UINT64_C(1000000000)
#define ACCOUNT_CHALLENGE_NS UINT64_C(30000000000)

static bool nonzero(const unsigned char *arr_data, size_t int_size)
{
    unsigned char ch_or = 0;
    for (size_t int_i = 0; int_i < int_size; ++int_i) ch_or |= arr_data[int_i];
    return ch_or != 0;
}

static const char *operation_name(uint16_t int_op)
{
    switch (int_op) {
    case AstAccount_HELLO: return "hello";
    case AstAccount_REGISTER: return "register";
    case AstAccount_LOGIN: return "login";
    case AstAccount_RESUME: return "resume";
    case AstAccount_LOGOUT: return "logout";
    case AstAccount_LEADERBOARD: return "leaderboard";
    case AstAccount_FORGET: return "forget";
    case AstAccount_KEEPALIVE: return "keepalive";
    default: return "close";
    }
}

static void phase(Client *str_client, LoginPhase int_phase, uint16_t int_status, bool b_error)
{
    mutex_lock(&str_client->str_mutex);
    str_client->str_login_view.int_phase = int_phase;
    str_client->str_login_view.int_status = int_status;
    str_client->str_login_view.b_error = b_error;
    mutex_unlock(&str_client->str_mutex);
}

static void profile_notice(Client *str_client, ProfileNotice int_notice, bool b_remembered)
{
    mutex_lock(&str_client->str_mutex);
    str_client->str_login_view.int_profile_notice = int_notice;
    str_client->str_login_view.b_remembered = b_remembered;
    mutex_unlock(&str_client->str_mutex);
}

/* Retain the token in owned memory long enough to request server revocation. */
static void forget_local(ClientAccount *str_account, Client *str_client)
{
    if (!str_account->b_profile_loaded) return;
    bool b_removed = profile_forget(&str_account->str_profile);
    profile_notice(str_client, b_removed ? PROFILE_NOTICE_LOCAL_ONLY : PROFILE_NOTICE_UNAVAILABLE, false);
}

static void clear_request(ClientAccount *str_account)
{
    OPENSSL_cleanse(&str_account->str_request, sizeof(str_account->str_request));
    str_account->b_pending = false;
}

static void clear_flow(ClientAccount *str_account)
{
    clear_request(str_account);
    OPENSSL_cleanse(&str_account->str_next, sizeof(str_account->str_next));
    OPENSSL_cleanse(str_account->arr_session, sizeof(str_account->arr_session));
    memset(str_account->arr_epoch, 0, sizeof(str_account->arr_epoch));
    memset(str_account->arr_challenge, 0, sizeof(str_account->arr_challenge));
    memset(str_account->arr_nonce, 0, sizeof(str_account->arr_nonce));
    str_account->int_seq = str_account->int_response_seq = 0;
    str_account->int_challenge_until = 0;
    str_account->b_authenticated = false;
    str_account->b_profile_revoke = false;
    str_account->b_end_after_result = false;
}

static bool begin_request(ClientAccount *str_account, uint16_t int_op)
{
    clear_request(str_account);
    AstAccount_Request *str_request = &str_account->str_request;
    if (str_account->int_seq == UINT64_MAX || !random_bytes(str_request->arr_request_nonce, 16) ||
        !nonzero(str_request->arr_request_nonce, 16)) return false;
    str_request->int_version = AstAccount_VERSION;
    str_request->int_operation = int_op;
    str_request->int_seq = ++str_account->int_seq;
    memcpy(str_request->arr_client_nonce, str_account->arr_nonce, 16);
    memcpy(str_request->arr_server_epoch, str_account->arr_epoch, 16);
    memcpy(str_request->arr_server_challenge, str_account->arr_challenge, 16);
    memcpy(str_request->arr_account_session, str_account->arr_session, 16);
    if (int_op == AstAccount_LOGIN || int_op == AstAccount_REGISTER) {
        const LoginCommand *str_command = &str_account->str_next;
        str_request->int_username_length = str_command->int_name_length;
        str_request->int_password_length = str_command->int_password_length;
        memcpy(str_request->arr_username, str_command->arr_name, str_command->int_name_length);
        memcpy(str_request->arr_password, str_command->arr_password, str_command->int_password_length);
        str_request->b_remember = str_command->b_remember;
    } else if (int_op == AstAccount_RESUME || (int_op == AstAccount_FORGET && !str_account->b_authenticated)) {
        memcpy(str_request->arr_resume_token, str_account->str_saved.arr_token, 32);
        str_request->b_remember = int_op == AstAccount_RESUME;
    }
    OPENSSL_cleanse(&str_account->str_next, sizeof(str_account->str_next));
    if (!account_request_valid(str_request)) {
        clear_request(str_account);
        return false;
    }
    str_account->int_deadline = monotonic_ns() + ACCOUNT_RPC_NS;
    str_account->int_next_send = 0;
    str_account->b_pending = true;
    return true;
}

static bool begin_hello(ClientAccount *str_account, Client *str_client, const LoginCommand *str_command)
{
    clear_flow(str_account);
    if (!random_bytes(str_account->arr_nonce, 16) || !nonzero(str_account->arr_nonce, 16)) return false;
    if (!begin_request(str_account, AstAccount_HELLO)) return false;
    str_account->str_next = *str_command;
    phase(str_client, LOGIN_PENDING, AstAccount_OK, false);
    return true;
}

void client_account_init(ClientAccount *str_account, Client *str_client)
{
    const Options *str_options = &str_client->str_options;
    str_account->str_profile = (ClientProfile){.int_directory = -1, .int_lock = -1};
    str_account->b_native_fault = strcmp(str_options->str_fault, "no-cert") == 0 ||
                                  strcmp(str_options->str_fault, "wrong-ca") == 0;
    if (str_account->b_native_fault) return;
    int int_loaded = profile_open(&str_account->str_profile, str_options) ?
        profile_load(&str_account->str_profile, &str_account->str_saved) : -1;
    str_account->b_profile_loaded = int_loaded == 1;
    if (int_loaded < 0) profile_notice(str_client, PROFILE_NOTICE_UNAVAILABLE, false);
    else if (int_loaded == 1) {
        mutex_lock(&str_client->str_mutex);
        memcpy(str_client->str_login_view.str_username, str_account->str_saved.arr_name,
               str_account->str_saved.int_name_length);
        str_client->str_login_view.b_remembered = true;
        mutex_unlock(&str_client->str_mutex);
    }
}

static bool request_from_form(ClientAccount *str_account, Client *str_client, LoginCommand *str_command)
{
    uint16_t int_op = str_command->int_operation;
    if (int_op == AstAccount_FORGET && !str_account->b_profile_loaded) return true;
    if (int_op != AstAccount_LOGIN && int_op != AstAccount_REGISTER && int_op != AstAccount_FORGET &&
        int_op != AstAccount_RESUME) return true;
    if (int_op == AstAccount_FORGET) forget_local(str_account, str_client);
    if (str_command->b_remember && str_account->str_profile.int_directory < 0) {
        str_command->b_remember = false;
        profile_notice(str_client, PROFILE_NOTICE_NOT_SAVED, false);
    }
    if (monotonic_ns() < str_account->int_challenge_until && nonzero(str_account->arr_challenge, 16)) {
        str_account->str_next = *str_command;
        phase(str_client, LOGIN_PENDING, AstAccount_OK, false);
        return begin_request(str_account, int_op);
    }
    return begin_hello(str_account, str_client, str_command);
}

static bool erase_profile(ClientAccount *str_account)
{
    bool b_ok = profile_forget(&str_account->str_profile);
    OPENSSL_cleanse(&str_account->str_saved, sizeof(str_account->str_saved));
    str_account->b_profile_loaded = false;
    return b_ok;
}

static bool request_from_game(ClientAccount *str_account, Client *str_client, uint16_t int_op)
{
    if (int_op != AstAccount_LOGOUT && int_op != AstAccount_FORGET && int_op != AstAccount_LEADERBOARD) return true;
    if (int_op == AstAccount_LOGOUT) {
        forget_local(str_account, str_client);
        str_account->b_reset_game = true;
        phase(str_client, LOGIN_ENDING, AstAccount_OK, false);
    } else if (int_op == AstAccount_FORGET) forget_local(str_account, str_client);
    return begin_request(str_account, int_op);
}

static bool start_automatic(ClientAccount *str_account, Client *str_client)
{
    LoginCommand str_command = {0};
    str_account->b_started = true;
    if (str_account->b_native_fault) return begin_hello(str_account, str_client, &str_command);
    if (str_account->b_profile_loaded) {
        const char *str_action = str_client->str_options.str_account_action;
        str_command.int_operation = str_action && strcmp(str_action, "forget") == 0 ? AstAccount_FORGET : AstAccount_RESUME;
        str_command.b_remember = str_command.int_operation == AstAccount_RESUME;
        return request_from_form(str_account, str_client, &str_command);
    }
    if (!str_client->str_options.b_headless) return true;
    fputs("CLIENT_ACCOUNT_INPUT_REQUIRED\n", stderr);
    return false;
}

static bool account_timeout(ClientAccount *str_account, Client *str_client, dds_entity_t int_writer, bool b_playing)
{
    uint16_t int_op = str_account->str_request.int_operation;
    if (int_op == AstAccount_HELLO) int_op = str_account->str_next.int_operation;
    if (int_op == AstAccount_FORGET && str_account->b_profile_loaded) {
        bool b_removed = erase_profile(str_account);
        profile_notice(str_client, b_removed ? PROFILE_NOTICE_LOCAL_ONLY : PROFILE_NOTICE_UNAVAILABLE, false);
        fputs("CLIENT_PROFILE forget=unconfirmed\n", stderr);
    }
    fprintf(stderr, "CLIENT_ACCOUNT operation=%s status=timeout\n", operation_name(int_op));
    if (b_playing && (int_op == AstAccount_LEADERBOARD || int_op == AstAccount_FORGET)) {
        clear_request(str_account);
        phase(str_client, LOGIN_ACTIVE, AstAccount_NOT_AUTHENTICATED, true);
        return true;
    }
    if (str_client->str_options.b_headless) return false;
    if (!client_account_close(str_account, int_writer)) return false;
    clear_flow(str_account);
    str_account->b_reset_game = true;
    phase(str_client, LOGIN_FORM, AstAccount_NOT_AUTHENTICATED, true);
    return true;
}

bool client_account_tick(ClientAccount *str_account, Client *str_client, dds_entity_t int_writer, bool b_playing)
{
    uint64_t int_now = monotonic_ns();
    if (str_account->b_pending && int_now >= str_account->int_deadline)
        return account_timeout(str_account, str_client, int_writer, b_playing);
    if (!str_account->b_pending) {
        LoginCommand str_command = {0};
        bool b_ok = true;
        if (str_account->b_native_fault && !str_account->b_started) b_ok = start_automatic(str_account, str_client);
        else if (login_take(str_client, &str_command)) {
            str_account->b_started = true;
            b_ok = str_account->b_authenticated ? request_from_game(str_account, str_client, str_command.int_operation) :
                                                request_from_form(str_account, str_client, &str_command);
        } else if (!str_account->b_started) b_ok = start_automatic(str_account, str_client);
        else if (str_account->b_authenticated && !b_playing && int_now >= str_account->int_keepalive)
            b_ok = begin_request(str_account, AstAccount_KEEPALIVE);
        OPENSSL_cleanse(&str_command, sizeof(str_command));
        if (!b_ok) return false;
    }
    if (!str_account->b_pending || int_now < str_account->int_next_send) return true;
    str_account->int_next_send = int_now + AST_JOIN_NS;
    return dds_ok(wire_write_account_request(int_writer, &str_account->str_request), "account_request_write");
}

static bool correlated(const ClientAccount *str_account, const AstAccount_Response *str_response)
{
    const AstAccount_Request *str_request = &str_account->str_request;
    if (!str_account->b_pending || !account_response_valid(str_response) ||
        str_response->int_seq <= str_account->int_response_seq ||
        str_response->int_request_seq != str_request->int_seq ||
        str_response->int_operation != str_request->int_operation ||
        memcmp(str_response->arr_client_nonce, str_request->arr_client_nonce, 16) != 0 ||
        memcmp(str_response->arr_request_nonce, str_request->arr_request_nonce, 16) != 0) return false;
    if (str_response->int_operation == AstAccount_HELLO) return true;
    if (memcmp(str_response->arr_server_challenge, str_request->arr_server_challenge, 16) != 0) return false;
    return str_response->int_status != AstAccount_OK || memcmp(str_response->arr_server_epoch, str_account->arr_epoch, 16) == 0;
}

static void publish_account(Client *str_client, const AstAccount_Response *str_response, bool b_leaderboard)
{
    mutex_lock(&str_client->str_mutex);
    LoginView *str_view = &str_client->str_login_view;
    memset(str_view->str_username, 0, sizeof(str_view->str_username));
    memcpy(str_view->str_username, str_response->arr_username, str_response->int_username_length);
    str_view->int_own_best = str_response->int_own_best;
    if (b_leaderboard) {
        str_view->int_entries = str_response->int_entry_count;
        memcpy(str_view->arr_entries, str_response->arr_entries, sizeof(str_view->arr_entries));
    } else str_view->int_entries = 0;
    str_view->int_phase = LOGIN_ACTIVE;
    str_view->b_error = false;
    mutex_unlock(&str_client->str_mutex);
}

static bool keep_token(ClientAccount *str_account, Client *str_client, const AstAccount_Response *str_response)
{
    if (!str_response->b_token_present) {
        if (str_account->b_profile_loaded && !erase_profile(str_account))
            profile_notice(str_client, PROFILE_NOTICE_UNAVAILABLE, false);
        else if (str_account->str_profile.int_directory >= 0) profile_notice(str_client, PROFILE_NOTICE_NONE, false);
        return true;
    }
    RememberedUser str_new = {.int_name_length = (unsigned char)str_response->int_username_length};
    memcpy(str_new.arr_name, str_response->arr_username, str_response->int_username_length);
    memcpy(str_new.arr_token, str_response->arr_resume_token, sizeof(str_new.arr_token));
    bool b_saved = profile_save(&str_account->str_profile, &str_new);
    if (b_saved) {
        str_account->str_saved = str_new;
        str_account->b_profile_loaded = true;
    } else (void)erase_profile(str_account);
    OPENSSL_cleanse(&str_new, sizeof(str_new));
    profile_notice(str_client, b_saved ? PROFILE_NOTICE_NONE : PROFILE_NOTICE_NOT_SAVED, b_saved);
    printf("CLIENT_PROFILE saved=%u\n", (unsigned int)b_saved);
    return b_saved;
}

static uint16_t account_only_operation(const Client *str_client)
{
    const char *str_action = str_client->str_options.str_account_action;
    if (str_action && strcmp(str_action, "logout") == 0) return AstAccount_LOGOUT;
    if (str_action && strcmp(str_action, "forget") == 0) return AstAccount_FORGET;
    return AstAccount_LEADERBOARD;
}

static bool authenticated(ClientAccount *str_account, Client *str_client, const AstAccount_Response *str_response)
{
    memcpy(str_account->arr_session, str_response->arr_account_session, 16);
    str_account->b_authenticated = true;
    str_account->int_keepalive = monotonic_ns() + ACCOUNT_KEEPALIVE_NS;
    publish_account(str_client, str_response, false);
    if (!keep_token(str_account, str_client, str_response)) {
        str_account->b_profile_revoke = true;
        return begin_request(str_account, AstAccount_FORGET);
    }
    if (str_client->str_options.b_account_only) {
        str_account->b_end_after_result = true;
        return request_from_game(str_account, str_client, account_only_operation(str_client));
    }
    return true;
}

static bool failed_reply(ClientAccount *str_account, Client *str_client, uint16_t int_op, uint16_t int_status)
{
    if (int_op == AstAccount_RESUME && int_status == AstAccount_INVALID_CREDENTIALS) {
        bool b_removed = erase_profile(str_account);
        profile_notice(str_client, b_removed ? PROFILE_NOTICE_FORGOTTEN : PROFILE_NOTICE_UNAVAILABLE, false);
    }
    if (int_op == AstAccount_FORGET && str_account->b_profile_loaded) {
        bool b_removed = erase_profile(str_account);
        profile_notice(str_client, b_removed ? PROFILE_NOTICE_LOCAL_ONLY : PROFILE_NOTICE_UNAVAILABLE, false);
    }
    if (str_account->b_profile_revoke) {
        str_account->b_profile_revoke = false;
        fputs("CLIENT_PROFILE revocation=unconfirmed\n", stderr);
        if (str_account->b_authenticated && int_status != AstAccount_NOT_AUTHENTICATED) {
            if (str_client->str_options.b_account_only) return false;
            return true;
        }
    }
    if (str_client->str_options.b_headless) return false;
    if (int_op == AstAccount_LEADERBOARD || (int_op == AstAccount_FORGET && str_account->b_authenticated)) {
        if (int_status != AstAccount_NOT_AUTHENTICATED) {
            phase(str_client, LOGIN_ACTIVE, int_status, true);
            return true;
        }
    }
    if (int_status != AstAccount_RATE_LIMITED || str_account->b_authenticated) clear_flow(str_account);
    str_account->b_reset_game = true;
    phase(str_client, LOGIN_FORM, int_status, true);
    return true;
}

static void finish_account_only(Client *str_client)
{
    atomic_store(&str_client->b_account_complete, true);
    atomic_store(&str_client->b_stop, true);
}

static bool successful_control(ClientAccount *str_account, Client *str_client, const AstAccount_Response *str_response)
{
    uint16_t int_op = str_response->int_operation;
    bool b_end = str_account->b_end_after_result || (str_client->str_options.b_account_only && !str_account->b_authenticated);
    if (int_op == AstAccount_LEADERBOARD) {
        publish_account(str_client, str_response, true);
        printf("CLIENT_LEADERBOARD count=%u own_best=%" PRIu64 "\n", str_response->int_entry_count, str_response->int_own_best);
        if (str_client->str_options.b_headless) for (unsigned int int_i = 0; int_i < str_response->int_entry_count; ++int_i) {
            const AstAccount_LeaderEntry *str_entry = &str_response->arr_entries[int_i];
            printf("CLIENT_LEADER rank=%u name=%.*s best=%" PRIu64 "\n", int_i + 1,
                   (int)str_entry->int_username_length, (const char *)str_entry->arr_username, str_entry->int_best_score);
        }
    } else if (int_op == AstAccount_FORGET || int_op == AstAccount_LOGOUT) {
        bool b_recover = str_account->b_profile_revoke;
        bool b_removed = erase_profile(str_account);
        str_account->b_profile_revoke = false;
        profile_notice(str_client, b_recover ? PROFILE_NOTICE_NOT_SAVED :
            b_removed ? PROFILE_NOTICE_FORGOTTEN : PROFILE_NOTICE_UNAVAILABLE, false);
        if (int_op == AstAccount_LOGOUT || !str_account->b_authenticated) {
            clear_flow(str_account);
            str_account->b_reset_game = true;
            phase(str_client, LOGIN_FORM, AstAccount_OK, false);
        } else phase(str_client, LOGIN_ACTIVE, AstAccount_OK, false);
        if (b_recover && str_client->str_options.b_account_only) return false;
    }
    if (b_end) finish_account_only(str_client);
    return true;
}

bool client_account_receive(ClientAccount *str_account, Client *str_client, const AstAccount_Response *str_response)
{
    if (!correlated(str_account, str_response)) return true;
    uint16_t int_op = str_response->int_operation;
    uint16_t int_status = str_response->int_status;
    if (int_status == AstAccount_BUSY) return true;
    bool b_auth = int_op == AstAccount_REGISTER || int_op == AstAccount_LOGIN || int_op == AstAccount_RESUME;
    if (int_status == AstAccount_OK && b_auth && str_response->b_token_present != str_account->str_request.b_remember)
        return true;
    str_account->int_response_seq = str_response->int_seq;
    clear_request(str_account);
    if (int_op != AstAccount_KEEPALIVE || int_status != AstAccount_OK)
        printf("CLIENT_ACCOUNT operation=%s status=%u persisted=%u\n", operation_name(int_op), int_status,
               (unsigned int)str_response->b_persisted);
    if (int_status != AstAccount_OK) return failed_reply(str_account, str_client, int_op, int_status);
    if (int_op == AstAccount_HELLO) {
        memcpy(str_account->arr_epoch, str_response->arr_server_epoch, 16);
        memcpy(str_account->arr_challenge, str_response->arr_server_challenge, 16);
        str_account->int_challenge_until = monotonic_ns() + ACCOUNT_CHALLENGE_NS;
        if (!str_account->str_next.int_operation) return false;
        return begin_request(str_account, str_account->str_next.int_operation);
    }
    if (b_auth) return authenticated(str_account, str_client, str_response);
    str_account->int_keepalive = monotonic_ns() + ACCOUNT_KEEPALIVE_NS;
    return successful_control(str_account, str_client, str_response);
}

bool client_account_play(const ClientAccount *str_account, const Client *str_client)
{
    return str_account->b_authenticated && !str_client->str_options.b_account_only &&
        !str_account->b_profile_revoke &&
        (!str_account->b_pending || str_account->str_request.int_operation != AstAccount_LOGOUT);
}

bool client_account_close(ClientAccount *str_account, dds_entity_t int_writer)
{
    if (int_writer <= 0 || !nonzero(str_account->arr_challenge, 16)) return true;
    if (str_account->b_pending && str_account->str_request.int_operation == AstAccount_LOGOUT) return true;
    if (!begin_request(str_account, AstAccount_CLOSE)) return false;
    if (!dds_ok(wire_write_account_request(int_writer, &str_account->str_request), "account_close_write")) return false;
    dds_return_t int_result = dds_wait_for_acks(int_writer, DDS_SECS(1));
    if (int_result == DDS_RETCODE_TIMEOUT) {
        fputs("CLIENT_WARNING account_close_ack_timeout\n", stderr);
        return true;
    }
    return dds_ok(int_result, "account_close_ack");
}

void client_account_fini(ClientAccount *str_account)
{
    profile_close(&str_account->str_profile);
    OPENSSL_cleanse(str_account, sizeof(*str_account));
}
