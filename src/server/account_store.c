/* Private SQLite account and score persistence. Team + AI, 2026-11-05. */
#include "server/account_store.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "common/config.h"

#define DB_NAME "accounts.sqlite3"
#define LOCK_NAME "accounts.lock"
#define SALT_BYTES 16u
#define HASH_BYTES 32u
#define SCRYPT_N UINT64_C(131072)
#define SCRYPT_R UINT64_C(8)
#define SCRYPT_P UINT64_C(1)
#define SCRYPT_MEMORY UINT64_C(167772160)
#define TOKEN_LIFETIME UINT64_C(604800)

_Static_assert(AST_IDENTITIES == 6, "schema requires six provisioned identities");
_Static_assert(ACCOUNT_STORE_LEADERBOARD_MAX == 10, "leaderboard SQL uses LIMIT 10");

/* Match the approved schema. Existing files must pass version/integrity checks
 * before any operation can write to them. */
static const char str_schema[] =
    "BEGIN IMMEDIATE;"
    "CREATE TABLE accounts ("
      "account_id INTEGER PRIMARY KEY CHECK(account_id > 0),"
      "username TEXT NOT NULL UNIQUE CHECK(length(username) BETWEEN 3 AND 24 "
        "AND username NOT GLOB '*[^a-z0-9_]*' AND instr(username,char(0))=0),"
      "identity_channel INTEGER NOT NULL CHECK(identity_channel BETWEEN 1 AND 6),"
      "password_salt BLOB NOT NULL CHECK(length(password_salt)=16),"
      "password_verifier BLOB NOT NULL CHECK(length(password_verifier)=32),"
      "scrypt_n INTEGER NOT NULL CHECK(scrypt_n=131072),"
      "scrypt_r INTEGER NOT NULL CHECK(scrypt_r=8),"
      "scrypt_p INTEGER NOT NULL CHECK(scrypt_p=1),"
      "password_generation INTEGER NOT NULL DEFAULT 1 CHECK(password_generation>0),"
      "created_unix_s INTEGER NOT NULL CHECK(created_unix_s>=0),"
      "UNIQUE(account_id,identity_channel));"
    "CREATE TABLE best_scores ("
      "account_id INTEGER PRIMARY KEY REFERENCES accounts(account_id),"
      "best_score INTEGER NOT NULL CHECK(best_score BETWEEN 0 AND 4294967295),"
      "source_round_key BLOB NOT NULL CHECK(length(source_round_key)=24),"
      "source_game_session BLOB NOT NULL CHECK(length(source_game_session)=8),"
      "updated_unix_s INTEGER NOT NULL CHECK(updated_unix_s>=0));"
    "CREATE TABLE resume_tokens ("
      "account_id INTEGER PRIMARY KEY,"
      "token_hash BLOB NOT NULL UNIQUE CHECK(length(token_hash)=32),"
      "identity_channel INTEGER NOT NULL CHECK(identity_channel BETWEEN 1 AND 6),"
      "password_generation INTEGER NOT NULL CHECK(password_generation>0),"
      "issued_unix_s INTEGER NOT NULL CHECK(issued_unix_s>=0),"
      "expires_unix_s INTEGER NOT NULL CHECK(expires_unix_s>issued_unix_s),"
      "FOREIGN KEY(account_id,identity_channel) REFERENCES accounts(account_id,identity_channel));"
    "PRAGMA user_version=2; COMMIT;";


/* Run only after the legacy database passed validation and its exclusive lock is held. */
static const char str_migrate_accounts[] =
    "BEGIN IMMEDIATE;"
    "CREATE TABLE accounts_v2 ("
      "account_id INTEGER PRIMARY KEY CHECK(account_id > 0),"
      "username TEXT NOT NULL UNIQUE CHECK(length(username) BETWEEN 3 AND 24 "
        "AND username NOT GLOB '*[^a-z0-9_]*' AND instr(username,char(0))=0),"
      "identity_channel INTEGER NOT NULL CHECK(identity_channel BETWEEN 1 AND 6),"
      "password_salt BLOB NOT NULL CHECK(length(password_salt)=16),"
      "password_verifier BLOB NOT NULL CHECK(length(password_verifier)=32),"
      "scrypt_n INTEGER NOT NULL CHECK(scrypt_n=131072),"
      "scrypt_r INTEGER NOT NULL CHECK(scrypt_r=8),"
      "scrypt_p INTEGER NOT NULL CHECK(scrypt_p=1),"
      "password_generation INTEGER NOT NULL DEFAULT 1 CHECK(password_generation>0),"
      "created_unix_s INTEGER NOT NULL CHECK(created_unix_s>=0),"
      "UNIQUE(account_id,identity_channel));"
    "INSERT INTO accounts_v2 SELECT * FROM accounts;"
    "DROP TRIGGER accounts_capacity;"
    "DROP TABLE accounts;"
    "ALTER TABLE accounts_v2 RENAME TO accounts;"
    "PRAGMA user_version=2;";

static bool directory_mode(int int_fd, bool b_leaf)
{
    struct stat str_info;
    if (fstat(int_fd, &str_info) != 0 || !S_ISDIR(str_info.st_mode)) return false;
    if (b_leaf) return str_info.st_uid == geteuid() && (str_info.st_mode & 07777) == 0700;
    if (str_info.st_uid != geteuid() && str_info.st_uid != 0) return false;
    /* A root-owned sticky parent such as /tmp cannot rename our owned child. */
    return !(str_info.st_mode & 0022) || (str_info.st_uid == 0 && (str_info.st_mode & S_ISVTX));
}

static int open_directory(const char *str_path)
{
    char str_real[PATH_MAX];
    if (!str_path || str_path[0] != '/' || !str_path[1] ||
        !realpath(str_path, str_real) || strcmp(str_path, str_real) != 0) return -1;
    int int_flags = O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC;
    int int_fd = open("/", int_flags);
    if (int_fd < 0) return -1;
    char *ptr_part = str_real + 1;
    for (;;) {
        char *ptr_next = strchr(ptr_part, '/');
        if (ptr_next) *ptr_next = '\0';
        int int_child = directory_mode(int_fd, false) ? openat(int_fd, ptr_part, int_flags) : -1;
        if (close(int_fd) != 0 || int_child < 0) {
            if (int_child >= 0) (void)close(int_child);
            return -1;
        }
        int_fd = int_child;
        if (!ptr_next) break;
        ptr_part = ptr_next + 1;
    }
    if (directory_mode(int_fd, true)) return int_fd;
    (void)close(int_fd);
    return -1;
}

static bool private_file(const struct stat *str_info)
{
    return S_ISREG(str_info->st_mode) && str_info->st_uid == geteuid() &&
        str_info->st_nlink == 1 && (str_info->st_mode & 07777) == 0600;
}

static bool file_check(int int_dir, const char *str_name, bool b_allow_missing)
{
    struct stat str_info;
    if (fstatat(int_dir, str_name, &str_info, AT_SYMLINK_NOFOLLOW) == 0)
        return private_file(&str_info);
    return b_allow_missing && errno == ENOENT;
}

static bool file_missing(int int_dir, const char *str_name)
{
    struct stat str_info;
    return fstatat(int_dir, str_name, &str_info, AT_SYMLINK_NOFOLLOW) != 0 && errno == ENOENT;
}

static bool acquire_files(AccountStore *str_store, bool *b_new)
{
    int int_dir = str_store->int_dir_fd;
    int int_flags = O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK;
    struct stat str_info;
    str_store->int_lock_fd = openat(int_dir, LOCK_NAME, O_RDWR | O_CREAT | int_flags, 0600);
    if (str_store->int_lock_fd < 0 || fstat(str_store->int_lock_fd, &str_info) != 0 ||
        !private_file(&str_info) || flock(str_store->int_lock_fd, LOCK_EX | LOCK_NB) != 0) return false;
    if (!file_missing(int_dir, DB_NAME "-wal") || !file_missing(int_dir, DB_NAME "-shm") ||
        !file_check(int_dir, DB_NAME "-journal", true)) return false;
    if (file_missing(int_dir, DB_NAME) && !file_missing(int_dir, DB_NAME "-journal")) return false;
    /* SQLite must never create a database at a path we did not create and check. */
    int int_db = openat(int_dir, DB_NAME, O_WRONLY | O_CREAT | O_EXCL | int_flags, 0600);
    if (int_db < 0) return errno == EEXIST && file_check(int_dir, DB_NAME, false);
    *b_new = true;
    bool b_private = fstat(int_db, &str_info) == 0 && private_file(&str_info);
    return close(int_db) == 0 && b_private;
}

static bool sql_exec(sqlite3 *ptr_db, const char *str_sql)
{
    return sqlite3_exec(ptr_db, str_sql, NULL, NULL, NULL) == SQLITE_OK;
}

static bool finish(sqlite3_stmt *ptr_stmt, bool b_ok)
{
    return sqlite3_finalize(ptr_stmt) == SQLITE_OK && b_ok;
}

static bool scalar(sqlite3 *ptr_db, const char *str_sql, const char *str_text, sqlite3_int64 int_value)
{
    sqlite3_stmt *ptr_stmt = NULL;
    if (sqlite3_prepare_v2(ptr_db, str_sql, -1, &ptr_stmt, NULL) != SQLITE_OK) return false;
    bool b_ok = sqlite3_step(ptr_stmt) == SQLITE_ROW;
    if (b_ok && str_text) {
        const unsigned char *ptr_text = sqlite3_column_text(ptr_stmt, 0);
        b_ok = ptr_text && strcmp((const char *)ptr_text, str_text) == 0;
    } else if (b_ok) {
        b_ok = sqlite3_column_type(ptr_stmt, 0) == SQLITE_INTEGER &&
            sqlite3_column_int64(ptr_stmt, 0) == int_value;
    }
    return finish(ptr_stmt, b_ok && sqlite3_step(ptr_stmt) == SQLITE_DONE);
}

static bool schema_valid(sqlite3 *ptr_db, int int_version)
{
    sqlite3_stmt *ptr_stmt = NULL;
    if (!scalar(ptr_db, "PRAGMA user_version", NULL, int_version) ||
        !scalar(ptr_db, "PRAGMA integrity_check(1)", "ok", 0) ||
        !scalar(ptr_db, "SELECT count(*) FROM sqlite_schema WHERE "
            "(type='table' AND name IN ('accounts','best_scores','resume_tokens')) "
            "OR (type='trigger' AND name='accounts_capacity')", NULL, int_version == 1 ? 4 : 3) ||
        sqlite3_prepare_v2(ptr_db, "PRAGMA foreign_key_check", -1, &ptr_stmt, NULL) != SQLITE_OK)
        return false;
    return finish(ptr_stmt, sqlite3_step(ptr_stmt) == SQLITE_DONE);
}

/* Foreign keys are restored on both commit and rollback; child rows keep their IDs. */
static bool migrate_accounts(sqlite3 *ptr_db)
{
    if (!sql_exec(ptr_db, "PRAGMA foreign_keys=OFF") ||
        !scalar(ptr_db, "PRAGMA foreign_keys", NULL, 0)) return false;
    bool b_ok = sql_exec(ptr_db, str_migrate_accounts) && schema_valid(ptr_db, 2);
    if (b_ok) b_ok = sql_exec(ptr_db, "COMMIT");
    if (!b_ok) (void)sql_exec(ptr_db, "ROLLBACK");
    bool b_foreign_keys = sql_exec(ptr_db, "PRAGMA foreign_keys=ON") &&
        scalar(ptr_db, "PRAGMA foreign_keys", NULL, 1);
    return b_ok && b_foreign_keys;
}

static bool open_database(AccountStore *str_store, const char *str_directory, bool b_new)
{
    char str_db_path[PATH_MAX];
    int int_length = snprintf(str_db_path, sizeof(str_db_path), "%s/%s", str_directory, DB_NAME);
    if (int_length < 0 || (size_t)int_length >= sizeof(str_db_path)) return false;
    int int_flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_NOFOLLOW;
    if (sqlite3_open_v2(str_db_path, &str_store->ptr_db, int_flags, NULL) != SQLITE_OK) return false;
    sqlite3 *ptr_db = str_store->ptr_db;
    if (sqlite3_db_readonly(ptr_db, "main") != 0 ||
        sqlite3_db_config(ptr_db, SQLITE_DBCONFIG_DEFENSIVE, 1, NULL) != SQLITE_OK ||
        sqlite3_db_config(ptr_db, SQLITE_DBCONFIG_TRUSTED_SCHEMA, 0, NULL) != SQLITE_OK ||
        sqlite3_busy_timeout(ptr_db, 100) != SQLITE_OK ||
        !sql_exec(ptr_db, "PRAGMA foreign_keys=ON; PRAGMA synchronous=FULL;") ||
        !scalar(ptr_db, "PRAGMA foreign_keys", NULL, 1) ||
        !scalar(ptr_db, "PRAGMA synchronous", NULL, 2) ||
        !scalar(ptr_db, "PRAGMA journal_mode", "delete", 0)) return false;
    if (b_new) return sql_exec(ptr_db, str_schema) && schema_valid(ptr_db, 2);
    if (scalar(ptr_db, "PRAGMA user_version", NULL, 2)) return schema_valid(ptr_db, 2);
    return schema_valid(ptr_db, 1) && migrate_accounts(ptr_db);
}

bool account_store_open(AccountStore *str_store, const char *str_directory)
{
    if (!str_store || str_store->ptr_db) return false;
    AccountStore str_open = {.int_dir_fd = open_directory(str_directory), .int_lock_fd = -1};
    bool b_new = false;
    if (str_open.int_dir_fd < 0) return false;
    if (!acquire_files(&str_open, &b_new) || !open_database(&str_open, str_directory, b_new)) {
        if (str_open.ptr_db) (void)sqlite3_close_v2(str_open.ptr_db);
        if (str_open.int_lock_fd >= 0) (void)close(str_open.int_lock_fd);
        (void)close(str_open.int_dir_fd);
        return false;
    }
    *str_store = str_open;
    return true;
}

bool account_store_close(AccountStore *str_store)
{
    if (!str_store || !str_store->ptr_db) return true;
    if (sqlite3_close(str_store->ptr_db) != SQLITE_OK) return false;
    bool b_lock_closed = close(str_store->int_lock_fd) == 0;
    bool b_dir_closed = close(str_store->int_dir_fd) == 0;
    *str_store = (AccountStore){0};
    return b_lock_closed && b_dir_closed;
}

static bool identity_valid(unsigned int int_identity)
{
    return int_identity > 0 && int_identity <= AST_IDENTITIES;
}

static bool account_id_valid(uint64_t int_account_id)
{
    return int_account_id > 0 && int_account_id <= INT64_MAX;
}

static bool username_valid(const char *str_name, size_t int_length)
{
    if (!str_name || int_length < 3 || int_length > ACCOUNT_STORE_USERNAME_MAX) return false;
    for (size_t int_i = 0; int_i < int_length; ++int_i) {
        char ch_value = str_name[int_i];
        if (!((ch_value >= 'a' && ch_value <= 'z') || (ch_value >= '0' && ch_value <= '9') ||
              ch_value == '_')) return false;
    }
    return true;
}

static bool password_valid(const uint8_t *arr_password, size_t int_length)
{
    if (!arr_password || int_length < 6 || int_length > 128) return false;
    for (size_t int_i = 0; int_i < int_length; ++int_i)
        if (arr_password[int_i] < 32 || arr_password[int_i] > 126) return false;
    return true;
}

static bool now_seconds(uint64_t *int_now)
{
    time_t int_current = time(NULL);
    if (int_current < 0 || (uint64_t)int_current > (uint64_t)INT64_MAX - TOKEN_LIFETIME) return false;
    *int_now = (uint64_t)int_current;
    return true;
}

static bool derive_password(const uint8_t *arr_password, size_t int_length,
    const uint8_t arr_salt[SALT_BYTES], uint8_t arr_hash[HASH_BYTES])
{
    return EVP_PBE_scrypt((const char *)arr_password, int_length, arr_salt, SALT_BYTES,
        SCRYPT_N, SCRYPT_R, SCRYPT_P, SCRYPT_MEMORY, arr_hash, HASH_BYTES) == 1;
}

static bool hash_token(const uint8_t arr_token[ACCOUNT_STORE_TOKEN_BYTES], uint8_t arr_hash[HASH_BYTES])
{
    const uint8_t arr_zero[ACCOUNT_STORE_TOKEN_BYTES] = {0};
    unsigned int int_bytes = 0;
    return CRYPTO_memcmp(arr_token, arr_zero, sizeof(arr_zero)) != 0 &&
        EVP_Digest(arr_token, ACCOUNT_STORE_TOKEN_BYTES, arr_hash, &int_bytes, EVP_sha256(), NULL) == 1 &&
        int_bytes == HASH_BYTES;
}

#define RECORD_COLUMNS "a.account_id,a.username,coalesce(b.best_score,0),coalesce(b.updated_unix_s,0) "
#define BEST_JOIN " LEFT JOIN best_scores b ON b.account_id=a.account_id "

/* Column checks prevent malformed persisted data becoming wire/account state. */
static bool read_record(sqlite3_stmt *ptr_stmt, AccountRecord *str_record)
{
    if (sqlite3_column_type(ptr_stmt, 0) != SQLITE_INTEGER ||
        sqlite3_column_type(ptr_stmt, 1) != SQLITE_TEXT ||
        sqlite3_column_type(ptr_stmt, 2) != SQLITE_INTEGER ||
        sqlite3_column_type(ptr_stmt, 3) != SQLITE_INTEGER) return false;
    sqlite3_int64 int_id = sqlite3_column_int64(ptr_stmt, 0);
    const char *str_name = (const char *)sqlite3_column_text(ptr_stmt, 1);
    int int_name_length = sqlite3_column_bytes(ptr_stmt, 1);
    sqlite3_int64 int_score = sqlite3_column_int64(ptr_stmt, 2);
    sqlite3_int64 int_updated = sqlite3_column_int64(ptr_stmt, 3);
    if (int_id <= 0 || int_score < 0 || int_score > UINT32_MAX || int_updated < 0 ||
        !username_valid(str_name, (size_t)int_name_length)) return false;
    *str_record = (AccountRecord){.int_account_id = (uint64_t)int_id,
        .int_best_score = (uint32_t)int_score, .int_updated_unix_s = (uint64_t)int_updated};
    memcpy(str_record->str_username, str_name, (size_t)int_name_length);
    return true;
}

static StoreResult finish_transaction(sqlite3 *ptr_db, StoreResult int_result)
{
    if (int_result == STORE_OK && sql_exec(ptr_db, "COMMIT")) return STORE_OK;
    if (!sqlite3_get_autocommit(ptr_db) && !sql_exec(ptr_db, "ROLLBACK")) return STORE_ERROR;
    return int_result == STORE_OK ? STORE_ERROR : int_result;
}

static bool store_ready(const AccountStore *str_store)
{
    return str_store && str_store->ptr_db && sqlite3_get_autocommit(str_store->ptr_db);
}

typedef struct {
    AccountRecord str_account;
    uint8_t arr_salt[SALT_BYTES], arr_verifier[HASH_BYTES];
} PasswordRecord;

static bool read_blob(sqlite3_stmt *ptr_stmt, int int_col, uint8_t *arr_out, int int_bytes)
{
    if (sqlite3_column_type(ptr_stmt, int_col) != SQLITE_BLOB ||
        sqlite3_column_bytes(ptr_stmt, int_col) != int_bytes) return false;
    const void *ptr_blob = sqlite3_column_blob(ptr_stmt, int_col);
    if (!ptr_blob) return false;
    memcpy(arr_out, ptr_blob, (size_t)int_bytes);
    return true;
}

static bool read_password(sqlite3_stmt *ptr_stmt, PasswordRecord *str_password)
{
    return read_record(ptr_stmt, &str_password->str_account) &&
        read_blob(ptr_stmt, 4, str_password->arr_salt, SALT_BYTES) &&
        read_blob(ptr_stmt, 5, str_password->arr_verifier, HASH_BYTES) &&
        sqlite3_column_type(ptr_stmt, 6) == SQLITE_INTEGER &&
        sqlite3_column_type(ptr_stmt, 7) == SQLITE_INTEGER &&
        sqlite3_column_type(ptr_stmt, 8) == SQLITE_INTEGER &&
        sqlite3_column_int64(ptr_stmt, 6) == SCRYPT_N &&
        sqlite3_column_int64(ptr_stmt, 7) == SCRYPT_R &&
        sqlite3_column_int64(ptr_stmt, 8) == SCRYPT_P;
}

static bool password_lookup(sqlite3 *ptr_db, unsigned int int_identity, const char *str_name,
    PasswordRecord *str_password)
{
    const char str_sql[] = "SELECT " RECORD_COLUMNS
        ",a.password_salt,a.password_verifier,a.scrypt_n,a.scrypt_r,a.scrypt_p "
        "FROM accounts a " BEST_JOIN "WHERE a.username=?1 AND a.identity_channel=?2";
    sqlite3_stmt *ptr_stmt = NULL;
    if (sqlite3_prepare_v2(ptr_db, str_sql, -1, &ptr_stmt, NULL) != SQLITE_OK) return false;
    bool b_ok = sqlite3_bind_text(ptr_stmt, 1, str_name, -1, SQLITE_STATIC) == SQLITE_OK &&
        sqlite3_bind_int(ptr_stmt, 2, (int)int_identity) == SQLITE_OK;
    if (!b_ok) return finish(ptr_stmt, false);
    int int_step = sqlite3_step(ptr_stmt);
    if (int_step == SQLITE_DONE) return finish(ptr_stmt, true);
    return finish(ptr_stmt, int_step == SQLITE_ROW && read_password(ptr_stmt, str_password) &&
        sqlite3_step(ptr_stmt) == SQLITE_DONE);
}

static StoreResult verify_password(sqlite3 *ptr_db, unsigned int int_identity, const char *str_name,
    const uint8_t *arr_password, size_t int_length, AccountRecord *str_account)
{
    /* A missing name/binding still performs the same cost KDF using dummy data. */
    PasswordRecord str_saved = {0};
    uint8_t arr_verifier[HASH_BYTES] = {0};
    bool b_ok = password_lookup(ptr_db, int_identity, str_name, &str_saved) &&
        derive_password(arr_password, int_length, str_saved.arr_salt, arr_verifier);
    int int_match = CRYPTO_memcmp(arr_verifier, str_saved.arr_verifier, HASH_BYTES);
    bool b_match = b_ok && int_match == 0 && str_saved.str_account.int_account_id != 0;
    if (b_match) *str_account = str_saved.str_account;
    OPENSSL_cleanse(arr_verifier, sizeof(arr_verifier));
    OPENSSL_cleanse(&str_saved, sizeof(str_saved));
    return !b_ok ? STORE_ERROR : b_match ? STORE_OK : STORE_INVALID_CREDENTIALS;
}

static StoreResult insert_account(sqlite3 *ptr_db, unsigned int int_identity, const char *str_name,
    uint64_t int_now, PasswordRecord *str_password)
{
    const char str_sql[] = "INSERT INTO accounts(username,identity_channel,password_salt,"
        "password_verifier,scrypt_n,scrypt_r,scrypt_p,created_unix_s) "
        "SELECT ?1,?2,?3,?4,?5,?6,?7,?8 WHERE NOT EXISTS "
        "(SELECT 1 FROM accounts WHERE username=?1) "
        "ON CONFLICT DO NOTHING";
    sqlite3_stmt *ptr_stmt = NULL;
    if (sqlite3_prepare_v2(ptr_db, str_sql, -1, &ptr_stmt, NULL) != SQLITE_OK) return STORE_ERROR;
    bool b_ok = sqlite3_bind_text(ptr_stmt, 1, str_name, -1, SQLITE_STATIC) == SQLITE_OK &&
        sqlite3_bind_int(ptr_stmt, 2, (int)int_identity) == SQLITE_OK &&
        sqlite3_bind_blob(ptr_stmt, 3, str_password->arr_salt, SALT_BYTES, SQLITE_STATIC) == SQLITE_OK &&
        sqlite3_bind_blob(ptr_stmt, 4, str_password->arr_verifier, HASH_BYTES, SQLITE_STATIC) == SQLITE_OK &&
        sqlite3_bind_int64(ptr_stmt, 5, SCRYPT_N) == SQLITE_OK &&
        sqlite3_bind_int64(ptr_stmt, 6, SCRYPT_R) == SQLITE_OK &&
        sqlite3_bind_int64(ptr_stmt, 7, SCRYPT_P) == SQLITE_OK &&
        sqlite3_bind_int64(ptr_stmt, 8, (sqlite3_int64)int_now) == SQLITE_OK &&
        sqlite3_step(ptr_stmt) == SQLITE_DONE;
    if (!finish(ptr_stmt, b_ok)) return STORE_ERROR;
    if (!sqlite3_changes(ptr_db)) return STORE_REGISTRATION_DENIED;
    sqlite3_int64 int_id = sqlite3_last_insert_rowid(ptr_db);
    if (sqlite3_changes(ptr_db) != 1 || int_id <= 0) return STORE_ERROR;
    str_password->str_account.int_account_id = (uint64_t)int_id;
    memcpy(str_password->str_account.str_username, str_name, strlen(str_name));
    return STORE_OK;
}

static bool save_token_hash(sqlite3 *ptr_db, uint64_t int_account_id, unsigned int int_identity,
    uint64_t int_now, const uint8_t arr_hash[HASH_BYTES])
{
    const char str_sql[] = "INSERT INTO resume_tokens(account_id,token_hash,identity_channel,"
        "password_generation,issued_unix_s,expires_unix_s) "
        "SELECT account_id,?1,identity_channel,password_generation,?2,?3 FROM accounts "
        "WHERE account_id=?4 AND identity_channel=?5 ON CONFLICT(account_id) DO UPDATE SET "
        "token_hash=excluded.token_hash,password_generation=excluded.password_generation,"
        "issued_unix_s=excluded.issued_unix_s,expires_unix_s=excluded.expires_unix_s";
    sqlite3_stmt *ptr_stmt = NULL;
    if (sqlite3_prepare_v2(ptr_db, str_sql, -1, &ptr_stmt, NULL) != SQLITE_OK) return false;
    bool b_ok = sqlite3_bind_blob(ptr_stmt, 1, arr_hash, HASH_BYTES, SQLITE_STATIC) == SQLITE_OK &&
        sqlite3_bind_int64(ptr_stmt, 2, (sqlite3_int64)int_now) == SQLITE_OK &&
        sqlite3_bind_int64(ptr_stmt, 3, (sqlite3_int64)(int_now + TOKEN_LIFETIME)) == SQLITE_OK &&
        sqlite3_bind_int64(ptr_stmt, 4, (sqlite3_int64)int_account_id) == SQLITE_OK &&
        sqlite3_bind_int(ptr_stmt, 5, (int)int_identity) == SQLITE_OK &&
        sqlite3_step(ptr_stmt) == SQLITE_DONE && sqlite3_changes(ptr_db) == 1;
    return finish(ptr_stmt, b_ok);
}

static bool issue_token(sqlite3 *ptr_db, unsigned int int_identity, uint64_t int_now,
    AccountGrant *str_grant)
{
    uint8_t arr_hash[HASH_BYTES] = {0};
    bool b_ok = RAND_bytes(str_grant->arr_token, sizeof(str_grant->arr_token)) == 1 &&
        hash_token(str_grant->arr_token, arr_hash) &&
        save_token_hash(ptr_db, str_grant->str_account.int_account_id, int_identity, int_now, arr_hash);
    OPENSSL_cleanse(arr_hash, sizeof(arr_hash));
    str_grant->b_token_present = b_ok;
    if (b_ok) str_grant->int_token_expiry_unix_s = int_now + TOKEN_LIFETIME;
    return b_ok;
}

static bool delete_token(sqlite3 *ptr_db, uint64_t int_account_id, unsigned int int_identity)
{
    sqlite3_stmt *ptr_stmt = NULL;
    if (sqlite3_prepare_v2(ptr_db, "DELETE FROM resume_tokens WHERE account_id=?1 AND identity_channel=?2",
        -1, &ptr_stmt, NULL) != SQLITE_OK) return false;
    bool b_ok = sqlite3_bind_int64(ptr_stmt, 1, (sqlite3_int64)int_account_id) == SQLITE_OK &&
        sqlite3_bind_int(ptr_stmt, 2, (int)int_identity) == SQLITE_OK && sqlite3_step(ptr_stmt) == SQLITE_DONE;
    return finish(ptr_stmt, b_ok);
}

StoreResult account_store_auth(AccountStore *str_store, bool b_register, unsigned int int_identity,
    const char *str_username, const uint8_t *arr_password, size_t int_password_length, bool b_remember,
    AccountGrant *str_grant)
{
    if (!str_grant) return STORE_ERROR;
    *str_grant = (AccountGrant){0};
    if (!store_ready(str_store)) return STORE_ERROR;
    if (!identity_valid(int_identity) || !str_username ||
        !username_valid(str_username, strnlen(str_username, ACCOUNT_STORE_USERNAME_MAX + 1)) ||
        !password_valid(arr_password, int_password_length)) return STORE_INVALID_CREDENTIALS;

    sqlite3 *ptr_db = str_store->ptr_db;
    PasswordRecord str_password = {0};
    AccountGrant str_result = {0};
    StoreResult int_result = STORE_OK;
    uint64_t int_now = 0;
    if (b_register) {
        if (RAND_bytes(str_password.arr_salt, sizeof(str_password.arr_salt)) != 1 ||
            !derive_password(arr_password, int_password_length, str_password.arr_salt,
                str_password.arr_verifier)) int_result = STORE_ERROR;
    } else {
        int_result = verify_password(ptr_db, int_identity, str_username, arr_password,
            int_password_length, &str_password.str_account);
    }
    if (int_result == STORE_OK) {
        if (!now_seconds(&int_now) || !sql_exec(ptr_db, "BEGIN IMMEDIATE")) int_result = STORE_ERROR;
        else {
            if (b_register) int_result = insert_account(ptr_db, int_identity, str_username, int_now, &str_password);
            str_result.str_account = str_password.str_account;
            if (int_result == STORE_OK) {
                bool b_ok = b_remember ? issue_token(ptr_db, int_identity, int_now, &str_result) :
                    delete_token(ptr_db, str_result.str_account.int_account_id, int_identity);
                if (!b_ok) int_result = STORE_ERROR;
            }
            int_result = finish_transaction(ptr_db, int_result);
        }
    }
    if (int_result == STORE_OK) *str_grant = str_result;
    OPENSSL_cleanse(&str_result, sizeof(str_result));
    OPENSSL_cleanse(&str_password, sizeof(str_password));
    return int_result;
}

static StoreResult one_record(sqlite3_stmt *ptr_stmt, bool b_bound, AccountRecord *str_account)
{
    if (!b_bound) { (void)finish(ptr_stmt, false); return STORE_ERROR; }
    int int_step = sqlite3_step(ptr_stmt);
    if (int_step == SQLITE_DONE)
        return finish(ptr_stmt, true) ? STORE_INVALID_CREDENTIALS : STORE_ERROR;
    bool b_ok = int_step == SQLITE_ROW && read_record(ptr_stmt, str_account) &&
        sqlite3_step(ptr_stmt) == SQLITE_DONE;
    return finish(ptr_stmt, b_ok) ? STORE_OK : STORE_ERROR;
}

static StoreResult find_token(sqlite3 *ptr_db, unsigned int int_identity, const uint8_t *arr_token,
    uint64_t int_now, AccountRecord *str_account)
{
    const uint8_t arr_zero[ACCOUNT_STORE_TOKEN_BYTES] = {0};
    if (!arr_token || CRYPTO_memcmp(arr_token, arr_zero, sizeof(arr_zero)) == 0)
        return STORE_INVALID_CREDENTIALS;
    const char str_sql[] = "SELECT " RECORD_COLUMNS "FROM resume_tokens t "
        "JOIN accounts a ON a.account_id=t.account_id AND a.identity_channel=t.identity_channel "
        BEST_JOIN "WHERE t.token_hash=?1 AND t.identity_channel=?2 AND t.issued_unix_s<=?3 "
        "AND t.expires_unix_s>?3 AND t.password_generation=a.password_generation";
    uint8_t arr_hash[HASH_BYTES] = {0};
    sqlite3_stmt *ptr_stmt = NULL;
    StoreResult int_result = STORE_ERROR;
    if (hash_token(arr_token, arr_hash) &&
        sqlite3_prepare_v2(ptr_db, str_sql, -1, &ptr_stmt, NULL) == SQLITE_OK) {
        bool b_bound = sqlite3_bind_blob(ptr_stmt, 1, arr_hash, HASH_BYTES, SQLITE_STATIC) == SQLITE_OK &&
            sqlite3_bind_int(ptr_stmt, 2, (int)int_identity) == SQLITE_OK &&
            sqlite3_bind_int64(ptr_stmt, 3, (sqlite3_int64)int_now) == SQLITE_OK;
        int_result = one_record(ptr_stmt, b_bound, str_account);
    }
    OPENSSL_cleanse(arr_hash, sizeof(arr_hash));
    return int_result;
}

StoreResult account_store_resume(AccountStore *str_store, unsigned int int_identity,
    const uint8_t arr_token[ACCOUNT_STORE_TOKEN_BYTES], AccountGrant *str_grant)
{
    if (!str_grant) return STORE_ERROR;
    *str_grant = (AccountGrant){0};
    if (!store_ready(str_store)) return STORE_ERROR;
    if (!identity_valid(int_identity)) return STORE_INVALID_CREDENTIALS;
    sqlite3 *ptr_db = str_store->ptr_db;
    uint64_t int_now = 0;
    if (!now_seconds(&int_now) || !sql_exec(ptr_db, "BEGIN IMMEDIATE")) return STORE_ERROR;
    AccountGrant str_result = {0};
    StoreResult int_result = find_token(ptr_db, int_identity, arr_token, int_now, &str_result.str_account);
    if (int_result == STORE_OK && !issue_token(ptr_db, int_identity, int_now, &str_result))
        int_result = STORE_ERROR;
    int_result = finish_transaction(ptr_db, int_result);
    if (int_result == STORE_OK) *str_grant = str_result;
    OPENSSL_cleanse(&str_result, sizeof(str_result));
    return int_result;
}

static StoreResult find_account(sqlite3 *ptr_db, uint64_t int_account_id, unsigned int int_identity,
    AccountRecord *str_account)
{
    const char str_sql[] = "SELECT " RECORD_COLUMNS "FROM accounts a " BEST_JOIN
        "WHERE a.account_id=?1 AND (?2=0 OR a.identity_channel=?2)";
    sqlite3_stmt *ptr_stmt = NULL;
    if (sqlite3_prepare_v2(ptr_db, str_sql, -1, &ptr_stmt, NULL) != SQLITE_OK) return STORE_ERROR;
    bool b_bound = sqlite3_bind_int64(ptr_stmt, 1, (sqlite3_int64)int_account_id) == SQLITE_OK &&
        sqlite3_bind_int(ptr_stmt, 2, (int)int_identity) == SQLITE_OK;
    return one_record(ptr_stmt, b_bound, str_account);
}

StoreResult account_store_revoke(AccountStore *str_store, unsigned int int_identity,
    uint64_t int_account_id, const uint8_t *arr_token)
{
    if (!store_ready(str_store)) return STORE_ERROR;
    if (!identity_valid(int_identity) || (arr_token ? int_account_id != 0 : !account_id_valid(int_account_id)))
        return STORE_INVALID_CREDENTIALS;
    sqlite3 *ptr_db = str_store->ptr_db;
    uint64_t int_now = 0;
    if ((arr_token && !now_seconds(&int_now)) || !sql_exec(ptr_db, "BEGIN IMMEDIATE")) return STORE_ERROR;
    AccountRecord str_account = {0};
    StoreResult int_result = arr_token ? find_token(ptr_db, int_identity, arr_token, int_now, &str_account) :
        find_account(ptr_db, int_account_id, int_identity, &str_account);
    if (int_result == STORE_OK && !delete_token(ptr_db, str_account.int_account_id, int_identity))
        int_result = STORE_ERROR;
    return finish_transaction(ptr_db, int_result);
}

StoreResult account_store_leaderboard(AccountStore *str_store, uint64_t int_account_id,
    AccountRecord *str_own, AccountRecord arr_rows[ACCOUNT_STORE_LEADERBOARD_MAX], size_t *int_count)
{
    if (!str_own || !arr_rows || !int_count) return STORE_ERROR;
    *str_own = (AccountRecord){0};
    memset(arr_rows, 0, sizeof(*arr_rows) * ACCOUNT_STORE_LEADERBOARD_MAX);
    *int_count = 0;
    if (!store_ready(str_store)) return STORE_ERROR;
    if (!account_id_valid(int_account_id)) return STORE_INVALID_CREDENTIALS;
    sqlite3 *ptr_db = str_store->ptr_db;
    AccountRecord str_own_row = {0};
    StoreResult int_result = find_account(ptr_db, int_account_id, 0, &str_own_row);
    if (int_result != STORE_OK) return int_result;

    const char str_sql[] = "SELECT " RECORD_COLUMNS "FROM accounts a " BEST_JOIN
        "ORDER BY coalesce(b.best_score,0) DESC,a.username COLLATE BINARY ASC LIMIT 10";
    sqlite3_stmt *ptr_stmt = NULL;
    if (sqlite3_prepare_v2(ptr_db, str_sql, -1, &ptr_stmt, NULL) != SQLITE_OK) return STORE_ERROR;
    AccountRecord arr_result[ACCOUNT_STORE_LEADERBOARD_MAX] = {0};
    size_t int_rows = 0;
    int int_step;
    while ((int_step = sqlite3_step(ptr_stmt)) == SQLITE_ROW) {
        if (int_rows == ACCOUNT_STORE_LEADERBOARD_MAX || !read_record(ptr_stmt, &arr_result[int_rows])) {
            (void)finish(ptr_stmt, false);
            return STORE_ERROR;
        }
        ++int_rows;
    }
    if (!finish(ptr_stmt, int_step == SQLITE_DONE)) return STORE_ERROR;
    *str_own = str_own_row;
    memcpy(arr_rows, arr_result, sizeof(arr_result));
    *int_count = int_rows;
    return STORE_OK;
}

StoreResult account_store_score(AccountStore *str_store, const AccountScore *str_score)
{
    if (!store_ready(str_store) || !str_score || !account_id_valid(str_score->int_account_id))
        return STORE_ERROR;
    uint64_t int_now = 0;
    if (!now_seconds(&int_now)) return STORE_ERROR;
    const char str_sql[] = "INSERT INTO best_scores(account_id,best_score,source_round_key,"
        "source_game_session,updated_unix_s) VALUES(?1,?2,?3,?4,?5) "
        "ON CONFLICT(account_id) DO UPDATE SET best_score=excluded.best_score,"
        "source_round_key=excluded.source_round_key,source_game_session=excluded.source_game_session,"
        "updated_unix_s=excluded.updated_unix_s WHERE excluded.best_score>best_scores.best_score";
    sqlite3_stmt *ptr_stmt = NULL;
    if (sqlite3_prepare_v2(str_store->ptr_db, str_sql, -1, &ptr_stmt, NULL) != SQLITE_OK) return STORE_ERROR;
    bool b_ok = sqlite3_bind_int64(ptr_stmt, 1, (sqlite3_int64)str_score->int_account_id) == SQLITE_OK &&
        sqlite3_bind_int64(ptr_stmt, 2, (sqlite3_int64)str_score->int_score) == SQLITE_OK &&
        sqlite3_bind_blob(ptr_stmt, 3, str_score->arr_round_key, sizeof(str_score->arr_round_key), SQLITE_STATIC) == SQLITE_OK &&
        sqlite3_bind_blob(ptr_stmt, 4, str_score->arr_game_session, sizeof(str_score->arr_game_session), SQLITE_STATIC) == SQLITE_OK &&
        sqlite3_bind_int64(ptr_stmt, 5, (sqlite3_int64)int_now) == SQLITE_OK &&
        sqlite3_step(ptr_stmt) == SQLITE_DONE;
    return finish(ptr_stmt, b_ok) ? STORE_OK : STORE_ERROR;
}
