/* Atomic Linux-home profiles store a resume token, never a password. Team/Codex, 2026-11-05. */
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <openssl/crypto.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include "client/profile.h"
#include "common/account_paths.h"
#include "common/account_protocol.h"

static const unsigned char arr_magic[] = {'A','S','T','P','R','O','F',1};
_Static_assert(sizeof(RememberedUser) == 57, "unexpected profile format padding");

static bool private_file(const struct stat *str_stat)
{
    return S_ISREG(str_stat->st_mode) && str_stat->st_uid == geteuid() &&
        (str_stat->st_mode & 07777) == 0600 && str_stat->st_nlink == 1;
}

/* The lock is held for the client lifetime, including load, rotation and logout. */
bool profile_open(ClientProfile *str_profile, const Options *str_options)
{
    *str_profile = (ClientProfile){.int_directory = -1, .int_lock = -1};
    char str_default[PATH_MAX];
    const char *str_directory = str_options->str_profile_dir;
    if (!str_directory) {
        if (!account_private_dir(str_options->str_security_dir, "profiles", str_default)) return false;
        str_directory = str_default;
    } else if (mkdir(str_directory, 0700) != 0 && errno != EEXIST) return false;
    int int_directory = open(str_directory, O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
    struct stat str_stat;
    if (int_directory < 0) return false;
    str_profile->int_directory = int_directory;
    if (fstat(int_directory, &str_stat) != 0 || str_stat.st_uid != geteuid() ||
        (str_stat.st_mode & 07777) != 0700) goto fail;
    char str_scope[65];
    if (!account_scope(str_options->str_security_dir, str_scope)) goto fail;
    /* Include the trust scope even in explicitly configured shared directories. */
    char str_lock[96];
    snprintf(str_profile->str_file, sizeof(str_profile->str_file), "p%u-%.64s.profile", str_options->int_identity, str_scope);
    snprintf(str_lock, sizeof(str_lock), "l%u-%.64s.lock", str_options->int_identity, str_scope);
    str_profile->int_lock = openat(int_directory, str_lock, O_CREAT | O_RDWR | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK, 0600);
    if (str_profile->int_lock < 0 || fstat(str_profile->int_lock, &str_stat) != 0 ||
        !private_file(&str_stat) || flock(str_profile->int_lock, LOCK_EX | LOCK_NB) != 0) goto fail;
    return true;
fail:
    profile_close(str_profile);
    return false;
}

static bool profile_user_valid(const RememberedUser *str_user)
{
    size_t int_length = str_user->int_name_length;
    return account_username_valid(str_user->arr_name, int_length) &&
        account_zero(str_user->arr_name + int_length, sizeof(str_user->arr_name) - int_length) &&
        !account_zero(str_user->arr_token, sizeof(str_user->arr_token));
}

static bool transfer(int int_fd, unsigned char *arr_bytes, size_t int_size, bool b_write)
{
    for (size_t int_done = 0; int_done < int_size;) {
        ssize_t int_n = b_write ? write(int_fd, arr_bytes + int_done, int_size - int_done) :
                                 read(int_fd, arr_bytes + int_done, int_size - int_done);
        if (int_n < 0 && errno == EINTR) continue;
        if (int_n <= 0) return false;
        int_done += (size_t)int_n;
    }
    return true;
}

int profile_load(const ClientProfile *str_profile, RememberedUser *str_user)
{
    OPENSSL_cleanse(str_user, sizeof(*str_user));
    if (str_profile->int_directory < 0) return -1;
    int int_fd = openat(str_profile->int_directory, str_profile->str_file, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (int_fd < 0) return errno == ENOENT ? 0 : -1;
    struct stat str_stat;
    unsigned char arr_bytes[sizeof(arr_magic) + sizeof(*str_user)] = {0};
    bool b_ok = fstat(int_fd, &str_stat) == 0 && private_file(&str_stat) &&
        str_stat.st_size == (off_t)sizeof(arr_bytes) && transfer(int_fd, arr_bytes, sizeof(arr_bytes), false) &&
        memcmp(arr_bytes, arr_magic, sizeof(arr_magic)) == 0;
    if (close(int_fd) != 0) b_ok = false;
    if (b_ok) memcpy(str_user, arr_bytes + sizeof(arr_magic), sizeof(*str_user));
    OPENSSL_cleanse(arr_bytes, sizeof(arr_bytes));
    if (b_ok && profile_user_valid(str_user)) return 1;
    OPENSSL_cleanse(str_user, sizeof(*str_user));
    return -1;
}

static int existing_file(const ClientProfile *str_profile)
{
    struct stat str_stat;
    if (str_profile->int_directory < 0) return -1;
    if (fstatat(str_profile->int_directory, str_profile->str_file, &str_stat, AT_SYMLINK_NOFOLLOW) != 0)
        return errno == ENOENT ? 0 : -1;
    return private_file(&str_stat) ? 1 : -1;
}

bool profile_save(const ClientProfile *str_profile, const RememberedUser *str_user)
{
    if (existing_file(str_profile) < 0 || !profile_user_valid(str_user)) return false;
    uint64_t int_temp;
    if (!random_bytes(&int_temp, sizeof(int_temp))) return false;
    char str_temp[48];
    snprintf(str_temp, sizeof(str_temp), ".new-%016" PRIx64 ".profile", int_temp);
    int int_directory = str_profile->int_directory;
    int int_fd = openat(int_directory, str_temp, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (int_fd < 0) return false;
    unsigned char arr_bytes[sizeof(arr_magic) + sizeof(*str_user)];
    memcpy(arr_bytes, arr_magic, sizeof(arr_magic));
    memcpy(arr_bytes + sizeof(arr_magic), str_user, sizeof(*str_user));
    bool b_ok = fchmod(int_fd, 0600) == 0 && transfer(int_fd, arr_bytes, sizeof(arr_bytes), true) && fsync(int_fd) == 0;
    OPENSSL_cleanse(arr_bytes, sizeof(arr_bytes));
    if (close(int_fd) != 0) b_ok = false;
    if (!b_ok || renameat(int_directory, str_temp, int_directory, str_profile->str_file) != 0) {
        (void)unlinkat(int_directory, str_temp, 0);
        return false;
    }
    return fsync(int_directory) == 0;
}

bool profile_forget(const ClientProfile *str_profile)
{
    int int_exists = existing_file(str_profile);
    return int_exists == 0 || (int_exists > 0 &&
        unlinkat(str_profile->int_directory, str_profile->str_file, 0) == 0 && fsync(str_profile->int_directory) == 0);
}

void profile_close(ClientProfile *str_profile)
{
    if (str_profile->int_lock >= 0) (void)close(str_profile->int_lock);
    if (str_profile->int_directory >= 0) (void)close(str_profile->int_directory);
    str_profile->int_lock = str_profile->int_directory = -1;
}
