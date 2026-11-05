/* Offline account persistence and failure tests. Team + AI, 2026-11-05.
 * GNU linker wrappers inject RNG failure and expiry in this executable only. */
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
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define DB_NAME "accounts.sqlite3"
#define JOURNAL_NAME DB_NAME "-journal"
#define CHECK(b_test) do { \
    if (!(b_test)) { \
        fprintf(stderr, "Account store test failed on line %d: %s\n", __LINE__, #b_test); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static char str_root[] = "/tmp/dds-account-store-test.XXXXXX";
static int int_root_fd = -1;
static bool b_root_created;
static AccountStore str_store;
static int int_fail_random_call;
static time_t int_forced_time;

static const char *const arr_bad_dirs[] = {
    "unsafe-mode", "unknown-schema", "corrupt", "empty", "symlink-db", "orphan-journal"
};

int __real_RAND_bytes(unsigned char *arr_data, int int_length);
time_t __real_time(time_t *int_result);

int __wrap_RAND_bytes(unsigned char *arr_data, int int_length)
{
    if (int_fail_random_call && --int_fail_random_call == 0) return 0;
    return __real_RAND_bytes(arr_data, int_length);
}

time_t __wrap_time(time_t *int_result)
{
    if (!int_forced_time) return __real_time(int_result);
    if (int_result) *int_result = int_forced_time;
    return int_forced_time;
}

static bool remove_if_present(int int_dir, const char *str_name, int int_flags)
{
    return unlinkat(int_dir, str_name, int_flags) == 0 || errno == ENOENT;
}

static bool remove_store_files(int int_dir)
{
    const char *const arr_names[] = {
        DB_NAME, "accounts.lock", JOURNAL_NAME, DB_NAME "-wal", DB_NAME "-shm"
    };
    bool b_ok = true;
    for (size_t int_i = 0; int_i < sizeof(arr_names) / sizeof(arr_names[0]); ++int_i)
        if (!remove_if_present(int_dir, arr_names[int_i], 0)) b_ok = false;
    return b_ok;
}

/* Only remove the known fixture files below the directory created by mkdtemp. */
static bool cleanup(void)
{
    bool b_ok = account_store_close(&str_store);
    if (int_root_fd >= 0) {
        for (size_t int_i = 0; int_i < sizeof(arr_bad_dirs) / sizeof(arr_bad_dirs[0]); ++int_i) {
            const char *str_name = arr_bad_dirs[int_i];
            int int_dir = openat(int_root_fd, str_name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
            if (int_dir >= 0) {
                if (!remove_store_files(int_dir)) b_ok = false;
                if (close(int_dir) != 0) b_ok = false;
                if (!remove_if_present(int_root_fd, str_name, AT_REMOVEDIR)) b_ok = false;
            } else if (errno != ENOENT) b_ok = false;
        }
        if (!remove_if_present(int_root_fd, "symlink-dir", 0)) b_ok = false;
        if (!remove_store_files(int_root_fd)) b_ok = false;
        if (close(int_root_fd) != 0) b_ok = false;
        int_root_fd = -1;
    }
    if (b_root_created) {
        if (rmdir(str_root) != 0) b_ok = false;
        b_root_created = false;
    }
    return b_ok;
}

static void cleanup_at_exit(void)
{
    if (!cleanup()) fputs("Account store fixture cleanup failed\n", stderr);
}

static void fixture_path(char str_path[PATH_MAX], const char *str_name, const char *str_suffix)
{
    int int_length = snprintf(str_path, PATH_MAX, "%s/%s%s", str_root, str_name, str_suffix);
    CHECK(int_length >= 0 && int_length < PATH_MAX);
}

static void test_registration(uint8_t arr_password[16], AccountGrant *str_alpha, AccountGrant *str_bravo)
{
    uint8_t arr_wrong[16];
    AccountGrant str_unused;
    CHECK(RAND_bytes(arr_password, 16) == 1);
    for (size_t int_i = 0; int_i < 16; ++int_i)
        arr_password[int_i] = (uint8_t)('a' + arr_password[int_i] % 26);
    memcpy(arr_wrong, arr_password, sizeof(arr_wrong));
    arr_wrong[0] = arr_wrong[0] == 'a' ? 'b' : 'a';
    CHECK(account_store_auth(&str_store, true, 1, "alpha", arr_password, 16, true, str_alpha) == STORE_OK);
    CHECK(str_alpha->str_account.int_account_id && str_alpha->b_token_present);
    CHECK(account_store_auth(&str_store, false, 1, "alpha", arr_wrong, 16, false, &str_unused) == STORE_INVALID_CREDENTIALS);
    CHECK(account_store_auth(&str_store, false, 2, "alpha", arr_password, 16, false, &str_unused) == STORE_INVALID_CREDENTIALS);
    CHECK(account_store_auth(&str_store, false, 1, "missing", arr_password, 16, false, &str_unused) == STORE_INVALID_CREDENTIALS);
    CHECK(account_store_auth(&str_store, true, 2, "alpha", arr_password, 16, false, &str_unused) == STORE_REGISTRATION_DENIED);
    CHECK(account_store_auth(&str_store, true, 1, "alpha", arr_password, 16, false, &str_unused) == STORE_REGISTRATION_DENIED);
    CHECK(account_store_auth(&str_store, true, 2, "bravo", arr_password, 16, false, str_bravo) == STORE_OK);
    CHECK(str_bravo->str_account.int_account_id != str_alpha->str_account.int_account_id && !str_bravo->b_token_present);

    int_fail_random_call = 2;
    CHECK(account_store_auth(&str_store, true, 3, "charlie", arr_password, 6, true, &str_unused) == STORE_ERROR);
    CHECK(!str_unused.b_token_present && str_unused.str_account.int_account_id == 0);
    CHECK(account_store_auth(&str_store, true, 3, "charlie", arr_password, 6, false, &str_unused) == STORE_OK);
    CHECK(account_store_auth(&str_store, false, 3, "charlie", arr_password, 6, false, &str_unused) == STORE_OK);
    CHECK(account_store_auth(&str_store, false, 3, "charlie", arr_password, 5, false, &str_unused) == STORE_INVALID_CREDENTIALS);
    puts("PASS six-letter password registration/login; five-letter password rejected");
    OPENSSL_cleanse(arr_wrong, sizeof(arr_wrong));
    OPENSSL_cleanse(&str_unused, sizeof(str_unused));
    puts("PASS registration binding, generic rejection and rollback on RNG failure");
}

static void test_scores(uint64_t int_alpha, uint64_t int_bravo)
{
    AccountScore str_score = {.int_account_id = int_alpha, .int_score = 240,
        .arr_round_key = {1}, .arr_game_session = {2}};
    CHECK(account_store_score(&str_store, &str_score) == STORE_OK);
    str_score.arr_round_key[0] = 3;
    CHECK(account_store_score(&str_store, &str_score) == STORE_OK);
    str_score.int_score = 180;
    CHECK(account_store_score(&str_store, &str_score) == STORE_OK);
    str_score.int_account_id = int_bravo;
    CHECK(account_store_score(&str_store, &str_score) == STORE_OK);
    CHECK(account_store_close(&str_store));
    CHECK(account_store_open(&str_store, str_root));

    AccountRecord str_own, arr_rows[ACCOUNT_STORE_LEADERBOARD_MAX];
    size_t int_count = 0;
    CHECK(account_store_leaderboard(&str_store, int_alpha, &str_own, arr_rows, &int_count) == STORE_OK);
    CHECK(int_count == 3 && str_own.int_best_score == 240 && arr_rows[0].int_best_score == 240 && arr_rows[1].int_best_score == 180);
    CHECK(strcmp(arr_rows[0].str_username, "alpha") == 0 && strcmp(arr_rows[1].str_username, "bravo") == 0);
    puts("PASS score max and leaderboard persist across database reopen");
}

/* Hash actual stored values, including byte blobs, across migration without logging secrets. */
static void account_rows_digest(uint8_t arr_digest[32])
{
    const char *arr_sql[] = {"SELECT * FROM accounts ORDER BY account_id",
        "SELECT * FROM best_scores ORDER BY account_id", "SELECT * FROM resume_tokens ORDER BY account_id"};
    EVP_MD_CTX *str_hash = EVP_MD_CTX_new();
    CHECK(str_hash && EVP_DigestInit_ex(str_hash, EVP_sha256(), NULL) == 1);
    for (size_t int_i = 0; int_i < 3; ++int_i) {
        sqlite3_stmt *str_stmt = NULL;
        CHECK(sqlite3_prepare_v2(str_store.ptr_db, arr_sql[int_i], -1, &str_stmt, NULL) == SQLITE_OK);
        int int_step;
        while ((int_step = sqlite3_step(str_stmt)) == SQLITE_ROW) {
            for (int int_col = 0; int_col < sqlite3_column_count(str_stmt); ++int_col) {
                int int_type = sqlite3_column_type(str_stmt, int_col);
                const void *ptr_value = sqlite3_column_blob(str_stmt, int_col);
                int int_bytes = sqlite3_column_bytes(str_stmt, int_col);
                CHECK(EVP_DigestUpdate(str_hash, &int_type, sizeof(int_type)) == 1);
                CHECK(EVP_DigestUpdate(str_hash, &int_bytes, sizeof(int_bytes)) == 1);
                if (int_bytes) CHECK(EVP_DigestUpdate(str_hash, ptr_value, (size_t)int_bytes) == 1);
            }
        }
        CHECK(int_step == SQLITE_DONE && sqlite3_finalize(str_stmt) == SQLITE_OK);
    }
    unsigned int int_size = 0;
    CHECK(EVP_DigestFinal_ex(str_hash, arr_digest, &int_size) == 1 && int_size == 32);
    EVP_MD_CTX_free(str_hash);
}

static void test_legacy_migration(void)
{
    /* Restore both legacy restrictions on the existing fixture before reopening it. */
    const char *str_legacy = "CREATE UNIQUE INDEX legacy_identity_limit ON accounts(identity_channel);"
        "CREATE TRIGGER accounts_capacity BEFORE INSERT ON accounts "
        "WHEN (SELECT count(*) FROM accounts)>=6 "
        "BEGIN SELECT RAISE(ABORT,'account capacity reached'); END; PRAGMA user_version=1;";
    CHECK(sqlite3_exec(str_store.ptr_db, str_legacy, NULL, NULL, NULL) == SQLITE_OK);
    uint8_t arr_before[32], arr_after[32];
    account_rows_digest(arr_before);
    CHECK(account_store_close(&str_store) && account_store_open(&str_store, str_root));
    account_rows_digest(arr_after);
    CHECK(CRYPTO_memcmp(arr_before, arr_after, sizeof(arr_before)) == 0);
    CHECK(sqlite3_exec(str_store.ptr_db, "PRAGMA foreign_keys", NULL, NULL, NULL) == SQLITE_OK);
    puts("PASS legacy migration preserves every account/score/token field byte-for-byte");
}

static void test_many_accounts(void)
{
    const uint8_t arr_digits[] = "123456";
    AccountGrant str_grant = {0};
    for (unsigned int int_i = 0; int_i < 12; ++int_i) {
        char arr_name[24];
        CHECK(snprintf(arr_name, sizeof(arr_name), "additional_%u", int_i) > 0);
        CHECK(account_store_auth(&str_store, true, 1, arr_name, arr_digits, 6, false, &str_grant) == STORE_OK);
        CHECK(account_store_auth(&str_store, false, 1, arr_name, arr_digits, 6, false, &str_grant) == STORE_OK);
        CHECK(account_store_auth(&str_store, false, 2, arr_name, arr_digits, 6, false, &str_grant) == STORE_INVALID_CREDENTIALS);
    }
    CHECK(account_store_auth(&str_store, true, 2, "additional_0", arr_digits, 6, false, &str_grant) == STORE_REGISTRATION_DENIED);
    CHECK(account_store_close(&str_store) && account_store_open(&str_store, str_root));
    CHECK(account_store_auth(&str_store, false, 1, "additional_11", arr_digits, 6, false, &str_grant) == STORE_OK);
    OPENSSL_cleanse(&str_grant, sizeof(str_grant));
    puts("PASS twelve accounts on one player beyond old global cap; login/reopen/unique usernames and certificate binding");
}

static void test_stored_material(const AccountGrant *str_alpha)
{
    char str_path[PATH_MAX];
    fixture_path(str_path, DB_NAME, "");
    sqlite3 *ptr_db = NULL;
    CHECK(sqlite3_open_v2(str_path, &ptr_db, SQLITE_OPEN_READONLY | SQLITE_OPEN_NOFOLLOW, NULL) == SQLITE_OK);
    sqlite3_stmt *ptr_stmt = NULL;
    const char str_sql[] = "SELECT a.password_salt,a.password_verifier,b.source_round_key,t.token_hash "
        "FROM accounts a JOIN best_scores b USING(account_id) JOIN resume_tokens t USING(account_id) "
        "WHERE a.account_id=?1";
    CHECK(sqlite3_prepare_v2(ptr_db, str_sql, -1, &ptr_stmt, NULL) == SQLITE_OK);
    CHECK(sqlite3_bind_int64(ptr_stmt, 1, (sqlite3_int64)str_alpha->str_account.int_account_id) == SQLITE_OK);
    CHECK(sqlite3_step(ptr_stmt) == SQLITE_ROW);
    CHECK(sqlite3_column_bytes(ptr_stmt, 0) == 16 && sqlite3_column_bytes(ptr_stmt, 1) == 32);
    const uint8_t *arr_round = sqlite3_column_blob(ptr_stmt, 2);
    CHECK(arr_round && sqlite3_column_bytes(ptr_stmt, 2) == 24 && arr_round[0] == 1);
    const uint8_t *arr_hash = sqlite3_column_blob(ptr_stmt, 3);
    uint8_t arr_expected[32];
    unsigned int int_bytes = 0;
    CHECK(EVP_Digest(str_alpha->arr_token, sizeof(str_alpha->arr_token), arr_expected, &int_bytes,
        EVP_sha256(), NULL) == 1 && int_bytes == sizeof(arr_expected));
    CHECK(arr_hash && sqlite3_column_bytes(ptr_stmt, 3) == 32 &&
        CRYPTO_memcmp(arr_hash, arr_expected, sizeof(arr_expected)) == 0);
    OPENSSL_cleanse(arr_expected, sizeof(arr_expected));
    CHECK(sqlite3_step(ptr_stmt) == SQLITE_DONE);
    CHECK(sqlite3_finalize(ptr_stmt) == SQLITE_OK);
    CHECK(sqlite3_close(ptr_db) == SQLITE_OK);
    puts("PASS verifier storage, hashed token and original score provenance");
}

static void test_tokens(const uint8_t arr_password[16], const AccountGrant *str_alpha)
{
    AccountGrant str_rotated, str_unused;
    CHECK(account_store_resume(&str_store, 2, str_alpha->arr_token, &str_unused) == STORE_INVALID_CREDENTIALS);
    int_forced_time = (time_t)str_alpha->int_token_expiry_unix_s;
    CHECK(int_forced_time > 0);
    CHECK(account_store_resume(&str_store, 1, str_alpha->arr_token, &str_unused) == STORE_INVALID_CREDENTIALS);
    int_forced_time = 0;
    int_fail_random_call = 1;
    CHECK(account_store_resume(&str_store, 1, str_alpha->arr_token, &str_unused) == STORE_ERROR);
    CHECK(!str_unused.b_token_present);
    CHECK(account_store_resume(&str_store, 1, str_alpha->arr_token, &str_rotated) == STORE_OK);
    CHECK(str_rotated.b_token_present && CRYPTO_memcmp(str_alpha->arr_token, str_rotated.arr_token,
        sizeof(str_rotated.arr_token)) != 0);
    CHECK(account_store_resume(&str_store, 1, str_alpha->arr_token, &str_unused) == STORE_INVALID_CREDENTIALS);
    CHECK(account_store_revoke(&str_store, 2, 0, str_rotated.arr_token) == STORE_INVALID_CREDENTIALS);
    CHECK(account_store_revoke(&str_store, 1, 0, str_rotated.arr_token) == STORE_OK);
    CHECK(account_store_resume(&str_store, 1, str_rotated.arr_token, &str_unused) == STORE_INVALID_CREDENTIALS);

    CHECK(account_store_auth(&str_store, false, 1, "alpha", arr_password, 16, true, &str_rotated) == STORE_OK);
    CHECK(account_store_auth(&str_store, false, 1, "alpha", arr_password, 16, false, &str_unused) == STORE_OK);
    CHECK(!str_unused.b_token_present);
    CHECK(account_store_resume(&str_store, 1, str_rotated.arr_token, &str_unused) == STORE_INVALID_CREDENTIALS);
    CHECK(account_store_revoke(&str_store, 1, str_alpha->str_account.int_account_id, NULL) == STORE_OK);
    OPENSSL_cleanse(&str_rotated, sizeof(str_rotated));
    OPENSSL_cleanse(&str_unused, sizeof(str_unused));
    puts("PASS token expiry, binding, rotation, rollback and revocation");
}

static void fingerprint(int int_dir, const char *str_name, uint8_t arr_hash[32])
{
    int int_file = openat(int_dir, str_name, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    CHECK(int_file >= 0);
    EVP_MD_CTX *ptr_digest = EVP_MD_CTX_new();
    CHECK(ptr_digest && EVP_DigestInit_ex(ptr_digest, EVP_sha256(), NULL) == 1);
    char arr_data[4096];
    ssize_t int_read;
    size_t int_blocks = 0;
    while ((int_read = read(int_file, arr_data, sizeof(arr_data))) > 0) {
        CHECK(++int_blocks <= 64);
        CHECK(EVP_DigestUpdate(ptr_digest, arr_data, (size_t)int_read) == 1);
    }
    CHECK(int_read == 0 && close(int_file) == 0);
    unsigned int int_bytes = 0;
    CHECK(EVP_DigestFinal_ex(ptr_digest, arr_hash, &int_bytes) == 1 && int_bytes == 32);
    EVP_MD_CTX_free(ptr_digest);
    OPENSSL_cleanse(arr_data, sizeof(arr_data));
}

static void make_bad_file(int int_dir, const char *str_case, const char *str_file)
{
    if (strcmp(str_case, "unknown-schema") == 0) {
        char str_path[PATH_MAX];
        fixture_path(str_path, str_case, "/" DB_NAME);
        sqlite3 *ptr_db = NULL;
        CHECK(sqlite3_open_v2(str_path, &ptr_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL) == SQLITE_OK);
        CHECK(sqlite3_exec(ptr_db, "PRAGMA user_version=99", NULL, NULL, NULL) == SQLITE_OK);
        CHECK(sqlite3_close(ptr_db) == SQLITE_OK);
        return;
    }
    int int_file = openat(int_dir, str_file, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
    CHECK(int_file >= 0);
    if (strcmp(str_case, "corrupt") == 0 || strcmp(str_case, "orphan-journal") == 0) {
        const char str_marker[] = "invalid fixture bytes";
        CHECK(write(int_file, str_marker, sizeof(str_marker)) == (ssize_t)sizeof(str_marker));
    }
    if (strcmp(str_case, "unsafe-mode") == 0) CHECK(fchmod(int_file, 0644) == 0);
    CHECK(close(int_file) == 0);
}

static void test_bad_path(const char *str_case)
{
    CHECK(mkdirat(int_root_fd, str_case, 0700) == 0);
    int int_dir = openat(int_root_fd, str_case, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    CHECK(int_dir >= 0);
    char str_path[PATH_MAX];
    fixture_path(str_path, str_case, "");
    bool b_symlink = strcmp(str_case, "symlink-db") == 0;
    bool b_orphan = strcmp(str_case, "orphan-journal") == 0;
    const char *str_file = b_orphan ? JOURNAL_NAME : DB_NAME;
    uint8_t arr_before[32], arr_after[32];
    if (b_symlink) CHECK(symlinkat("../" DB_NAME, int_dir, DB_NAME) == 0);
    else {
        make_bad_file(int_dir, str_case, str_file);
        fingerprint(int_dir, str_file, arr_before);
    }
    CHECK(!account_store_open(&str_store, str_path));
    if (!b_symlink) {
        fingerprint(int_dir, str_file, arr_after);
        CHECK(CRYPTO_memcmp(arr_before, arr_after, sizeof(arr_before)) == 0);
    }
    struct stat str_info;
    if (b_orphan) CHECK(fstatat(int_dir, DB_NAME, &str_info, AT_SYMLINK_NOFOLLOW) != 0 && errno == ENOENT);
    if (b_symlink) CHECK(fstatat(int_dir, DB_NAME, &str_info, AT_SYMLINK_NOFOLLOW) == 0 && S_ISLNK(str_info.st_mode));
    CHECK(close(int_dir) == 0);
}

static void test_paths(void)
{
    uint8_t arr_before[32], arr_after[32];
    fingerprint(int_root_fd, DB_NAME, arr_before);
    for (size_t int_i = 0; int_i < sizeof(arr_bad_dirs) / sizeof(arr_bad_dirs[0]); ++int_i)
        test_bad_path(arr_bad_dirs[int_i]);
    CHECK(symlinkat(".", int_root_fd, "symlink-dir") == 0);
    char str_path[PATH_MAX];
    fixture_path(str_path, "symlink-dir", "");
    CHECK(!account_store_open(&str_store, str_path));
    fingerprint(int_root_fd, DB_NAME, arr_after);
    CHECK(CRYPTO_memcmp(arr_before, arr_after, sizeof(arr_before)) == 0);
    puts("PASS unsafe database paths refused and existing bytes preserved");
}

int main(void)
{
    umask(0077);
    CHECK(atexit(cleanup_at_exit) == 0);
    CHECK(mkdtemp(str_root) != NULL);
    b_root_created = true;
    int_root_fd = open(str_root, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    CHECK(int_root_fd >= 0 && account_store_open(&str_store, str_root));
    AccountStore str_duplicate = {0};
    CHECK(!account_store_open(&str_duplicate, str_root));
    struct stat str_info;
    CHECK(fstatat(int_root_fd, DB_NAME, &str_info, AT_SYMLINK_NOFOLLOW) == 0 &&
        (str_info.st_mode & 0777) == 0600);
    CHECK(fstatat(int_root_fd, "accounts.lock", &str_info, AT_SYMLINK_NOFOLLOW) == 0 &&
        (str_info.st_mode & 0777) == 0600);
    puts("PASS exclusive store lock and private file modes");

    uint8_t arr_password[16];
    AccountGrant str_alpha, str_bravo;
    test_registration(arr_password, &str_alpha, &str_bravo);
    test_scores(str_alpha.str_account.int_account_id, str_bravo.str_account.int_account_id);
    test_legacy_migration();
    test_stored_material(&str_alpha);
    test_tokens(arr_password, &str_alpha);
    test_many_accounts();
    OPENSSL_cleanse(arr_password, sizeof(arr_password));
    OPENSSL_cleanse(&str_alpha, sizeof(str_alpha));
    OPENSSL_cleanse(&str_bravo, sizeof(str_bravo));
    CHECK(account_store_close(&str_store));
    test_paths();
    CHECK(cleanup());
    return EXIT_SUCCESS;
}
