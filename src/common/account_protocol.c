/**
 * @file
 * @brief Validates account credentials and DDS account message fields.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
#include "common/account_protocol.h"

bool account_zero(const void *ptr_bytes, size_t int_length)
{
    const uint8_t *arr_bytes = ptr_bytes;
    uint8_t int_combined = 0;
    for (size_t int_i = 0; int_i < int_length; ++int_i) int_combined |= arr_bytes[int_i];
    return int_combined == 0;
}

bool account_username_valid(const uint8_t *arr_name, size_t int_length)
{
    if (!arr_name || int_length < 3 || int_length > 24) return false;
    for (size_t int_i = 0; int_i < int_length; ++int_i) {
        uint8_t ch = arr_name[int_i];
        if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_')) return false;
    }
    return true;
}

bool account_password_valid(const uint8_t *arr_password, size_t int_length)
{
    if (!arr_password || int_length < 6 || int_length > 128) return false;
    for (size_t int_i = 0; int_i < int_length; ++int_i)
        if (arr_password[int_i] < 32 || arr_password[int_i] > 126) return false;
    return true;
}

static bool known_operation(uint16_t int_operation)
{
    return int_operation >= AstAccount_HELLO && int_operation <= AstAccount_CLOSE;
}

/* Counted text has a single canonical representation for request retries. */
static bool clean_tail(const uint8_t *arr_bytes, size_t int_length, size_t int_size)
{
    return int_length <= int_size && account_zero(arr_bytes + int_length, int_size - int_length);
}

bool account_request_valid(const AstAccount_Request *str_req)
{
    if (!str_req || str_req->int_version != AstAccount_VERSION || !known_operation(str_req->int_operation) ||
        !str_req->int_seq || account_zero(str_req->arr_client_nonce, 16) ||
        account_zero(str_req->arr_request_nonce, 16)) return false;
    if (!clean_tail(str_req->arr_username, str_req->int_username_length, sizeof(str_req->arr_username)) ||
        !clean_tail(str_req->arr_password, str_req->int_password_length, sizeof(str_req->arr_password))) return false;
    const bool b_epoch = !account_zero(str_req->arr_server_epoch, 16);
    const bool b_challenge = !account_zero(str_req->arr_server_challenge, 16);
    const bool b_session = !account_zero(str_req->arr_account_session, 16);
    const bool b_token = !account_zero(str_req->arr_resume_token, 32);
    const bool b_text = str_req->int_username_length || str_req->int_password_length;
    if (str_req->int_operation == AstAccount_HELLO)
        return !b_epoch && !b_challenge && !b_session && !b_token && !b_text && !str_req->b_remember;
    if (!b_epoch || !b_challenge) return false;
    if (str_req->int_operation == AstAccount_LOGIN || str_req->int_operation == AstAccount_REGISTER)
        return !b_session && !b_token && account_username_valid(str_req->arr_username, str_req->int_username_length) &&
               account_password_valid(str_req->arr_password, str_req->int_password_length);
    if (b_text) return false;
    if (str_req->int_operation == AstAccount_RESUME) return !b_session && b_token && str_req->b_remember;
    if (str_req->b_remember) return false;
    if (str_req->int_operation == AstAccount_FORGET) return b_session != b_token;
    if (str_req->int_operation == AstAccount_CLOSE) return !b_token;
    return b_session && !b_token;
}

static bool entry_valid(const AstAccount_LeaderEntry *str_entry, bool b_used)
{
    if (!clean_tail(str_entry->arr_username, str_entry->int_username_length, sizeof(str_entry->arr_username)))
        return false;
    if (!b_used) return !str_entry->int_account_id && !str_entry->int_best_score &&
                        !str_entry->int_updated_unix_s && !str_entry->int_username_length;
    return str_entry->int_account_id && str_entry->int_account_id <= INT64_MAX &&
           str_entry->int_best_score <= UINT32_MAX && str_entry->int_updated_unix_s <= INT64_MAX &&
           account_username_valid(str_entry->arr_username, str_entry->int_username_length);
}

bool account_response_valid(const AstAccount_Response *str_rsp)
{
    if (!str_rsp || str_rsp->int_version != AstAccount_VERSION || !known_operation(str_rsp->int_operation) ||
        str_rsp->int_status > AstAccount_REGISTRATION_DENIED || !str_rsp->int_seq || !str_rsp->int_request_seq ||
        account_zero(str_rsp->arr_client_nonce, 16) || account_zero(str_rsp->arr_request_nonce, 16) ||
        str_rsp->int_entry_count > 10 ||
        !clean_tail(str_rsp->arr_username, str_rsp->int_username_length, sizeof(str_rsp->arr_username))) return false;
    const bool b_ok = str_rsp->int_status == AstAccount_OK;
    const uint16_t int_op = str_rsp->int_operation;
    const bool b_auth = b_ok && (int_op == AstAccount_REGISTER || int_op == AstAccount_LOGIN || int_op == AstAccount_RESUME);
    const bool b_board = b_ok && int_op == AstAccount_LEADERBOARD;
    const bool b_token = !account_zero(str_rsp->arr_resume_token, 32);
    const bool b_session = !account_zero(str_rsp->arr_account_session, 16);
    if (b_ok && (account_zero(str_rsp->arr_server_epoch, 16) || account_zero(str_rsp->arr_server_challenge, 16)))
        return false;
    if (b_session != b_auth || b_token != str_rsp->b_token_present || (b_token && !b_auth) ||
        (b_auth && int_op == AstAccount_RESUME && !b_token) || (str_rsp->int_token_expiry_unix_s != 0) != b_token ||
        str_rsp->int_token_expiry_unix_s > INT64_MAX || (!b_board && str_rsp->int_entry_count)) return false;
    const bool b_persisted = b_auth || b_board || (b_ok && (int_op == AstAccount_LOGOUT || int_op == AstAccount_FORGET));
    if (str_rsp->b_persisted != b_persisted) return false;
    if (b_auth || b_board) {
        if (!str_rsp->int_account_id || str_rsp->int_account_id > INT64_MAX || str_rsp->int_own_best > UINT32_MAX ||
            !account_username_valid(str_rsp->arr_username, str_rsp->int_username_length)) return false;
    } else if (str_rsp->int_account_id || str_rsp->int_own_best || str_rsp->int_username_length) return false;
    for (size_t int_i = 0; int_i < 10; ++int_i)
        if (!entry_valid(&str_rsp->arr_entries[int_i], int_i < str_rsp->int_entry_count)) return false;
    return true;
}
