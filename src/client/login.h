/**
 * @file
 * @brief Login commands, public UI state and the client login mailbox API.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
#ifndef AST_CLIENT_LOGIN_H
#define AST_CLIENT_LOGIN_H
#include "common/protocol.h"
#include "accounts.h"

enum { CLIENT_LOGIN_EVENT = 3, CLIENT_NAME_CAP = 24, CLIENT_PASSWORD_CAP = 128, CLIENT_LEADERS_CAP = 10 };
typedef enum { LOGIN_FORM, LOGIN_PENDING, LOGIN_ACTIVE, LOGIN_ENDING } LoginPhase;
typedef enum { PROFILE_NOTICE_NONE, PROFILE_NOTICE_UNAVAILABLE, PROFILE_NOTICE_NOT_SAVED,
               PROFILE_NOTICE_FORGOTTEN, PROFILE_NOTICE_LOCAL_ONLY } ProfileNotice;

typedef struct {
    uint16_t int_operation, int_name_length, int_password_length;
    unsigned char arr_name[CLIENT_NAME_CAP], arr_password[CLIENT_PASSWORD_CAP];
    bool b_remember;
} LoginCommand;

typedef struct {
    LoginPhase int_phase;
    ProfileNotice int_profile_notice;
    uint16_t int_status;
    bool b_error, b_remembered;
    char str_username[CLIENT_NAME_CAP + 1];
    uint64_t int_own_best;
    unsigned int int_entries;
    AstAccount_LeaderEntry arr_entries[CLIENT_LEADERS_CAP];
} LoginView;

struct Client;
/** @brief Check the operation and credential fields of a login command. */
bool login_command_valid(const LoginCommand *str_command);
/** @brief Copy a command into the client login mailbox. */
bool login_submit(struct Client *str_client, const LoginCommand *str_command);
/** @brief Consume the pending login command if one is available. */
bool login_take(struct Client *str_client, LoginCommand *str_command);
/** @brief Read headless account credentials from the supplied file descriptor. */
bool login_read_credentials(struct Client *str_client, int int_fd);
/** @brief Copy the public login state for rendering. */
void login_view(struct Client *str_client, LoginView *str_view);
/** @brief Return the display string for an account status code. */
const char *login_status_text(uint16_t int_status);
#endif
