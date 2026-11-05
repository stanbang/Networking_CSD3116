/* Derive stable state scope from the trusted CA and open owned directories. Team + AI, 2026-11-05. */
#include "common/account_paths.h"
#include <errno.h>
#include <fcntl.h>
#include <openssl/pem.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

bool account_scope(const char *str_security_dir, char arr_scope[65])
{
    if (!str_security_dir) return false;
    char arr_ca[PATH_MAX];
    int int_size = snprintf(arr_ca, sizeof(arr_ca), "%s/identity_ca/ca.crt", str_security_dir);
    if (int_size < 0 || (size_t)int_size >= sizeof(arr_ca)) return false;
    FILE *ptr_file = fopen(arr_ca, "r");
    if (!ptr_file) return false;
    X509 *ptr_ca = PEM_read_X509(ptr_file, NULL, NULL, NULL);
    bool b_closed = fclose(ptr_file) == 0;
    uint8_t arr_digest[32];
    unsigned int int_length = 0;
    bool b_ok = ptr_ca && b_closed &&
        X509_digest(ptr_ca, EVP_sha256(), arr_digest, &int_length) == 1 && int_length == sizeof(arr_digest);
    X509_free(ptr_ca);
    if (!b_ok) return false;
    const char arr_hex[] = "0123456789abcdef";
    for (size_t int_i = 0; int_i < sizeof(arr_digest); ++int_i) {
        arr_scope[2 * int_i] = arr_hex[arr_digest[int_i] >> 4];
        arr_scope[2 * int_i + 1] = arr_hex[arr_digest[int_i] & 15u];
    }
    arr_scope[64] = 0;
    return true;
}

static bool owned_dir(int int_fd, bool b_private)
{
    struct stat str_st;
    if (fstat(int_fd, &str_st) || !S_ISDIR(str_st.st_mode) || str_st.st_uid != geteuid()) return false;
    return b_private ? (str_st.st_mode & 0777) == 0700 : !(str_st.st_mode & 0022);
}

bool account_private_dir(const char *str_security_dir, const char *str_kind, char arr_path[PATH_MAX])
{
    if (strcmp(str_kind, "server") && strcmp(str_kind, "profiles")) return false;
    struct passwd str_pw, *ptr_pw = NULL;
    char arr_pw[16384], arr_scope[65], arr_leaf[80];
    if (getpwuid_r(geteuid(), &str_pw, arr_pw, sizeof(arr_pw), &ptr_pw) || !ptr_pw ||
        str_pw.pw_dir[0] != '/' || !account_scope(str_security_dir, arr_scope)) return false;
    int int_size = snprintf(arr_leaf, sizeof(arr_leaf), "%s-%s", str_kind, arr_scope);
    if (int_size < 0 || (size_t)int_size >= sizeof(arr_leaf)) return false;
    int_size = snprintf(arr_path, PATH_MAX, "%s/.local/state/dds-asteroids/%s", str_pw.pw_dir, arr_leaf);
    if (int_size < 0 || int_size >= PATH_MAX) return false;
    const int int_flags = O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC;
    int int_fd = open(str_pw.pw_dir, int_flags);
    if (int_fd < 0) return false;
    bool b_ok = owned_dir(int_fd, false);
    const char *arr_parts[] = {".local", "state", "dds-asteroids", arr_leaf};
    for (size_t int_i = 0; b_ok && int_i < sizeof(arr_parts) / sizeof(*arr_parts); ++int_i) {
        if (mkdirat(int_fd, arr_parts[int_i], 0700) && errno != EEXIST) { b_ok = false; break; }
        int int_next = openat(int_fd, arr_parts[int_i], int_flags);
        if (close(int_fd)) b_ok = false;
        int_fd = int_next;
        b_ok = b_ok && int_fd >= 0 && owned_dir(int_fd, int_i == 3);
    }
    if (int_fd >= 0 && close(int_fd)) b_ok = false;
    return b_ok;
}
