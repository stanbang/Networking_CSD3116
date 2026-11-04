/* Post-crypto sendmsg bit-flip shim, test only. Author: <team>/Codex; 2026-11-03.
 * Uses installed ddsi/q_protocol.h definitions, never application plaintext.
 * Build: cc -shared -fPIC -Wall -Wextra tests/rtps_fault.c -o build/rtps_fault.so -ldl -pthread -lcrypto
 */
#define _GNU_SOURCE
#include <dds/ddsi/q_protocol.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <openssl/evp.h>

#define TEST_PACKET_CAP 65536u
static ssize_t (*real_sendmsg)(int, const struct msghdr *, int);
static pthread_once_t once = PTHREAD_ONCE_INIT;
static atomic_ulong calls, mutations;
static atomic_flag captured = ATOMIC_FLAG_INIT;
static int (*real_decrypt_final)(EVP_CIPHER_CTX *, unsigned char *, int *);
static pthread_once_t crypto_once = PTHREAD_ONCE_INIT;
static atomic_ulong gcm_failures;

/* Resolve the actual OpenSSL finalizer called by native Cyclone crypto. */
static void resolve_decrypt(void)
{
    real_decrypt_final = (int (*)(EVP_CIPHER_CTX *, unsigned char *, int *))
        dlsym(RTLD_NEXT, "EVP_DecryptFinal_ex");
}

/* Test-only instrumentation: record real GCM failure without touching keys/data. */
int EVP_DecryptFinal_ex(EVP_CIPHER_CTX *context, unsigned char *out, int *length)
{
    if (pthread_once(&crypto_once, resolve_decrypt) || !real_decrypt_final) return 0;
    int result = real_decrypt_final(context, out, length);
    if (result <= 0 && EVP_CIPHER_CTX_get_mode(context) == EVP_CIPH_GCM_MODE) {
        unsigned long count = atomic_fetch_add(&gcm_failures, 1) + 1;
        if (count == 1) fprintf(stderr, "NATIVE_GCM_AUTH_FAILURE result=%d\n", result);
    }
    return result;
}

/* POSIX dlsym function resolution; no syscall is attempted until resolved. */
static void resolve_sendmsg(void)
{
    real_sendmsg = (ssize_t (*)(int, const struct msghdr *, int))dlsym(RTLD_NEXT, "sendmsg");
}

/* Parse bounded RTPS submessages; return an encrypted body byte after its size. */
static size_t cipher_offset(const unsigned char *bytes, size_t size)
{
    if (size < 24 || memcmp(bytes, "RTPS", 4)) return 0;
    for (size_t pos = 20; pos <= size - 4;) {
        size_t length = bytes[pos + 1] & SMFLAG_ENDIANNESS ?
            (size_t)bytes[pos + 2] | ((size_t)bytes[pos + 3] << 8) :
            ((size_t)bytes[pos + 2] << 8) | bytes[pos + 3];
        if (!length) length = size - pos - 4;
        if (length > size - pos - 4) return 0;
        if (bytes[pos] == SMID_SEC_BODY && length > 12) return pos + 8;
        pos += 4 + length;
    }
    return 0;
}

/* Capture only serialized ciphertext for independent byte comparison. */
static void record_packet(const char *suffix, const unsigned char *bytes, size_t size)
{
    const char *prefix = getenv("AST_FAULT_CAPTURE");
    char path[1024];
    if (!prefix) return;
    int length = snprintf(path, sizeof(path), "%s-%s.bin", prefix, suffix);
    if (length < 0 || (size_t)length >= sizeof(path)) return;
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    if (fd < 0) { perror("WIRE_FAULT capture open"); return; }
    size_t done = 0;
    while (done < size) {
        ssize_t written = write(fd, bytes + done, size - done);
        if (written <= 0) { perror("WIRE_FAULT capture write"); break; }
        done += (size_t)written;
    }
    if (close(fd)) perror("WIRE_FAULT capture close");
}

/* Arm-file gating permits real authentication/join before mutating wire bytes. */
ssize_t sendmsg(int fd, const struct msghdr *message, int flags)
{
    if (pthread_once(&once, resolve_sendmsg) || !real_sendmsg) { errno = ENOSYS; return -1; }
    atomic_fetch_add(&calls, 1);
    const char *arm = getenv("AST_FAULT_ARM");
    if (!arm || access(arm, F_OK)) return real_sendmsg(fd, message, flags);
    size_t size = 0;
    for (size_t i = 0; i < message->msg_iovlen; ++i) {
        if (message->msg_iov[i].iov_len > TEST_PACKET_CAP - size)
            return real_sendmsg(fd, message, flags);
        size += message->msg_iov[i].iov_len;
    }
    unsigned char *copy = malloc(size ? size : 1);
    if (!copy) return real_sendmsg(fd, message, flags);
    size_t cursor = 0;
    for (size_t i = 0; i < message->msg_iovlen; ++i) {
        memcpy(copy + cursor, message->msg_iov[i].iov_base, message->msg_iov[i].iov_len);
        cursor += message->msg_iov[i].iov_len;
    }
    size_t offset = cipher_offset(copy, size);
    if (!offset) { free(copy); return real_sendmsg(fd, message, flags); }
    bool first = !atomic_flag_test_and_set(&captured);
    if (first) record_packet("before", copy, size);
    bool observe = getenv("AST_FAULT_OBSERVE") != NULL;
    if (!observe) copy[offset] ^= 1;
    if (first) record_packet("after", copy, size);
    if (!observe) atomic_fetch_add(&mutations, 1);
    struct iovec vector = {.iov_base = copy, .iov_len = size};
    struct msghdr altered = *message;
    altered.msg_iov = &vector;
    altered.msg_iovlen = 1;
    ssize_t result = real_sendmsg(fd, &altered, flags);
    int saved_errno = errno;
    if (first) fprintf(stderr, "%s offset=%zu bytes=%zu send_result=%zd\n",
                       observe ? "WIRE_OBSERVE" : "WIRE_FAULT", offset, size, result);
    free(copy);
    errno = saved_errno;
    return result;
}

/* Counters prove this library intercepted actual middleware socket calls. */
__attribute__((destructor)) static void report(void)
{
    fprintf(stderr, "WIRE_FAULT_SUMMARY calls=%lu mutations=%lu\n",
            atomic_load(&calls), atomic_load(&mutations));
    fprintf(stderr, "NATIVE_GCM_SUMMARY failures=%lu\n", atomic_load(&gcm_failures));
}
