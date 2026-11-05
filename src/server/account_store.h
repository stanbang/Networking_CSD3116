/**
 * @file
 * @brief Storage-worker API for accounts, credentials and leaderboard persistence.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
#ifndef AST_ACCOUNT_STORE_H
#define AST_ACCOUNT_STORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ACCOUNT_STORE_USERNAME_MAX 24u
#define ACCOUNT_STORE_LEADERBOARD_MAX 10u
#define ACCOUNT_STORE_TOKEN_BYTES 32u

struct sqlite3;

typedef struct {
    struct sqlite3 *ptr_db;
    int int_dir_fd, int_lock_fd;
} AccountStore;

typedef enum {
    STORE_OK,
    STORE_INVALID_CREDENTIALS,
    STORE_REGISTRATION_DENIED,
    STORE_ERROR
} StoreResult;

typedef struct {
    uint64_t int_account_id, int_updated_unix_s;
    uint32_t int_best_score;
    char str_username[ACCOUNT_STORE_USERNAME_MAX + 1];
} AccountRecord;

typedef struct {
    AccountRecord str_account;
    bool b_token_present;
    uint8_t arr_token[ACCOUNT_STORE_TOKEN_BYTES];
    uint64_t int_token_expiry_unix_s;
} AccountGrant;

typedef struct {
    uint64_t int_account_id;
    uint32_t int_score;
    uint8_t arr_round_key[24], arr_game_session[8];
} AccountScore;

/**
 * @brief Open and migrate the SQLite account database inside a private directory.
 *
 * The worker owns the connection and every call after open. Set process umask
 * before creating threads. The absolute data directory must already be private.
 */
bool account_store_open(AccountStore *str_store, const char *str_directory);
/** @brief Close SQLite and release the account store's handles. */
bool account_store_close(AccountStore *str_store);

/**
 * @brief Register or authenticate an account and optionally issue a resume token.
 *
 * Username is NUL-terminated. Inputs and grant do not overlap. Grant is zero on
 * failure. Caller wipes the input password and returned grant.
 */
StoreResult account_store_auth(AccountStore *str_store, bool b_register,
    unsigned int int_identity, const char *str_username, const uint8_t *arr_password,
    size_t int_password_length, bool b_remember, AccountGrant *str_grant);
/** @brief Validate and rotate a remembered login token for the certified identity. */
StoreResult account_store_resume(AccountStore *str_store, unsigned int int_identity,
    const uint8_t arr_token[ACCOUNT_STORE_TOKEN_BYTES], AccountGrant *str_grant);

/**
 * @brief Revoke a remembered login grant by account or token.
 *
 * Revoke by authenticated account id or by token possession, never both.
 */
StoreResult account_store_revoke(AccountStore *str_store, unsigned int int_identity,
    uint64_t int_account_id, const uint8_t *arr_token);
/** @brief Read the current best scores and the requesting account's record. */
StoreResult account_store_leaderboard(AccountStore *str_store, uint64_t int_account_id,
    AccountRecord *str_own, AccountRecord arr_rows[ACCOUNT_STORE_LEADERBOARD_MAX],
    size_t *int_count);
/** @brief Persist a score while retaining the highest score for the account. */
StoreResult account_store_score(AccountStore *str_store, const AccountScore *str_score);

#endif
