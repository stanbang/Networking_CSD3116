/* Login input validation and an owned one-command mailbox. Team/Codex, 2026-11-05. */
#include <errno.h>
#include <openssl/crypto.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>
#include "client/client.h"
#include "common/account_protocol.h"

bool login_command_valid(const LoginCommand *str_command)
{
    uint16_t int_op = str_command->int_operation;
    if (int_op == AstAccount_LOGOUT || int_op == AstAccount_FORGET || int_op == AstAccount_LEADERBOARD)
        return !str_command->int_name_length && !str_command->int_password_length && !str_command->b_remember;
    return (int_op == AstAccount_LOGIN || int_op == AstAccount_REGISTER) &&
        account_username_valid(str_command->arr_name, str_command->int_name_length) &&
        account_password_valid(str_command->arr_password, str_command->int_password_length);
}

bool login_submit(Client *str_client, const LoginCommand *str_command)
{
    if (!login_command_valid(str_command) || atomic_load(&str_client->b_stop)) return false;
    mutex_lock(&str_client->str_mutex);
    bool b_ok = !str_client->b_login_command;
    if (b_ok) {
        str_client->str_login_command = *str_command;
        str_client->b_login_command = true;
    }
    mutex_unlock(&str_client->str_mutex);
    return b_ok && event_queue_mark(&str_client->str_events, CLIENT_LOGIN_EVENT);
}

bool login_take(Client *str_client, LoginCommand *str_command)
{
    mutex_lock(&str_client->str_mutex);
    bool b_found = str_client->b_login_command;
    if (b_found) {
        *str_command = str_client->str_login_command;
        OPENSSL_cleanse(&str_client->str_login_command, sizeof(str_client->str_login_command));
        str_client->b_login_command = false;
    }
    mutex_unlock(&str_client->str_mutex);
    return b_found;
}

/* No arbitrary-length getline, secret environment variables or argv values. */
static bool read_frame(int int_fd, unsigned char *arr_bytes, size_t int_cap, size_t *int_size)
{
    uint64_t int_deadline = monotonic_ns() + UINT64_C(3000000000);
    while (*int_size < int_cap && monotonic_ns() < int_deadline && !client_interrupted()) {
        struct pollfd str_poll = {.fd = int_fd, .events = POLLIN};
        int int_ready = poll(&str_poll, 1, 100);
        if (int_ready < 0 && errno == EINTR) continue;
        if (int_ready < 0) return false;
        if (!int_ready) continue;
        ssize_t int_n = read(int_fd, arr_bytes + *int_size, int_cap - *int_size);
        if (int_n < 0 && errno == EINTR) continue;
        if (int_n < 0) return false;
        if (!int_n) return true;
        *int_size += (size_t)int_n;
    }
    return false;
}

bool login_read_credentials(Client *str_client, int int_fd)
{
    unsigned char arr_bytes[CLIENT_NAME_CAP + CLIENT_PASSWORD_CAP + 3] = {0};
    size_t int_size = 0;
    LoginCommand str_command = {.int_operation = str_client->str_options.b_register ? AstAccount_REGISTER : AstAccount_LOGIN,
                                .b_remember = str_client->str_options.b_remember};
    bool b_ok = read_frame(int_fd, arr_bytes, sizeof(arr_bytes), &int_size);
    if (close(int_fd) != 0) b_ok = false;
    unsigned char *ptr_newline = memchr(arr_bytes, '\n', int_size);
    if (b_ok && ptr_newline && int_size && arr_bytes[int_size - 1] == '\n') {
        size_t int_name = (size_t)(ptr_newline - arr_bytes);
        size_t int_password = int_size - int_name - 1;
        if (int_password) --int_password;
        if (int_name <= CLIENT_NAME_CAP && int_password <= CLIENT_PASSWORD_CAP) {
            str_command.int_name_length = (uint16_t)int_name;
            str_command.int_password_length = (uint16_t)int_password;
            memcpy(str_command.arr_name, arr_bytes, int_name);
            memcpy(str_command.arr_password, ptr_newline + 1, int_password);
        }
    } else b_ok = false;
    if (b_ok) b_ok = login_submit(str_client, &str_command);
    OPENSSL_cleanse(&str_command, sizeof(str_command));
    OPENSSL_cleanse(arr_bytes, sizeof(arr_bytes));
    return b_ok;
}

void login_view(Client *str_client, LoginView *str_view)
{
    mutex_lock(&str_client->str_mutex);
    *str_view = str_client->str_login_view;
    mutex_unlock(&str_client->str_mutex);
}

const char *login_status_text(uint16_t int_status)
{
    switch (int_status) {
    case AstAccount_INVALID_REQUEST: return "Invalid account request";
    case AstAccount_INVALID_CREDENTIALS: return "Invalid credentials";
    case AstAccount_ALREADY_ACTIVE: return "Account already active";
    case AstAccount_RATE_LIMITED: return "Too many attempts. Try again shortly";
    case AstAccount_BUSY: return "Server busy. Try again shortly";
    case AstAccount_STORAGE_ERROR: return "Server account storage error";
    case AstAccount_NOT_AUTHENTICATED: return "Account service unavailable or expired";
    case AstAccount_CAPACITY: return "Account capacity reached";
    case AstAccount_UNSUPPORTED_VERSION: return "Account protocol version mismatch";
    case AstAccount_REGISTRATION_DENIED: return "Registration denied";
    default: return "Account request failed";
    }
}
