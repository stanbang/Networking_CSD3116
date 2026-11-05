/* Read-only OpenSSL definedness diagnostics, TEST ONLY. Team/Codex, 2026-11-04.
 * Preload only into a diagnostic DDS game process under Memcheck. Not production.
 * The installed openssl/evp.h declares this exact EVP_EncryptUpdate signature.
 * Only Valgrind V-bit metadata is queried; plaintext/ciphertext/key bytes are
 * never read, changed, printed or marked defined/undefined by this helper.
 * Build: cc -std=gnu11 -Wall -Wextra -Wformat=2 -Werror -fPIC -shared
 *        tests/crypto_definedness.c -o build/crypto_definedness.so -ldl -pthread
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <valgrind/memcheck.h>

#define PROBE_CHUNK_BYTES 256u
#define PROBE_BUFFER_LIMIT 16384u
#define PROBE_CALL_LIMIT 4096u

typedef int (*EncryptUpdate)(EVP_CIPHER_CTX *, unsigned char *, int *, const unsigned char *, int);
typedef struct {
    size_t int_checked, int_undefined;
    /* Native GET_VBITS codes:0=not Valgrind,1=success,3=unaddressable.
     * Probe-only codes:4=missing buffer,5=over cap,6=negative size,7=failed call. */
    unsigned int int_query;
} Definedness;
static EncryptUpdate str_real_update;
static pthread_once_t str_resolve_once = PTHREAD_ONCE_INIT;
static atomic_uint int_observations;

/* A missing real symbol aborts this diagnostic instead of inventing a result. */
static void resolve_update(void)
{
    void *ptr_symbol = dlsym(RTLD_NEXT, "EVP_EncryptUpdate");
    _Static_assert(sizeof(str_real_update) == sizeof(ptr_symbol), "unsupported function-pointer ABI");
    if (!ptr_symbol) { fputs("CRYPTO_DEFINEDNESS_INIT_FAILED\n", stderr); _Exit(127); }
    memcpy(&str_real_update, &ptr_symbol, sizeof(str_real_update));
}

/* Bound diagnostics globally without overflow, allocation or a new worker. */
static bool reserve_observation(void)
{
    unsigned int int_seen = atomic_load_explicit(&int_observations, memory_order_relaxed);
    while (int_seen < PROBE_CALL_LIMIT) {
        if (atomic_compare_exchange_weak_explicit(&int_observations, &int_seen, int_seen + 1,
                                                  memory_order_relaxed, memory_order_relaxed)) return true;
    }
    return false;
}

/* Query metadata in fixed stack chunks. No source bytes or V-bits are changed. */
static Definedness inspect_buffer(const unsigned char *arr_buffer, int int_length)
{
    Definedness str_result = {.int_query = 1};
    if (int_length < 0) { str_result.int_query = 6; return str_result; }
    if (!int_length) return str_result;
    if (!arr_buffer) { str_result.int_query = 4; return str_result; }
    if ((unsigned int)int_length > PROBE_BUFFER_LIMIT) { str_result.int_query = 5; return str_result; }
    for (size_t int_offset = 0; int_offset < (size_t)int_length;) {
        unsigned char arr_vbits[PROBE_CHUNK_BYTES] = {0};
        size_t int_size = (size_t)int_length - int_offset;
        if (int_size > sizeof(arr_vbits)) int_size = sizeof(arr_vbits);
        str_result.int_query = VALGRIND_GET_VBITS(arr_buffer + int_offset, arr_vbits, int_size);
        if (str_result.int_query != 1) return str_result;
        for (size_t int_index = 0; int_index < int_size; ++int_index)
            str_result.int_undefined += arr_vbits[int_index] != 0;
        str_result.int_checked += int_size;
        int_offset += int_size;
    }
    return str_result;
}

/* Exact pass-through: input observed before EVP; successful output after EVP.
 * AAD calls have NULL output and are labelled missing-buffer rather than clean.
 * Failed calls do not promise a valid output length, so it is never inspected. */
int EVP_EncryptUpdate(EVP_CIPHER_CTX *str_context, unsigned char *arr_output,
                      int *ptr_output_length, const unsigned char *arr_input, int int_input_length)
{
    int int_original_errno = errno;
    if (pthread_once(&str_resolve_once, resolve_update) != 0) {
        fputs("CRYPTO_DEFINEDNESS_ONCE_FAILED\n", stderr); _Exit(127);
    }
    bool b_observe = RUNNING_ON_VALGRIND && reserve_observation();
    Definedness str_input = {0};
    if (b_observe) str_input = inspect_buffer(arr_input, int_input_length);
    errno = int_original_errno;
    int int_result = str_real_update(str_context, arr_output, ptr_output_length, arr_input, int_input_length);
    int int_result_errno = errno;
    if (b_observe) {
        int int_output_length = -1;
        Definedness str_output = {.int_query = 7};
        if (int_result > 0 && ptr_output_length) {
            int_output_length = *ptr_output_length;
            str_output = inspect_buffer(arr_output, int_output_length);
        }
        fprintf(stderr, "CRYPTO_DEFINEDNESS input_length=%d input_checked=%zu input_undefined=%zu input_query=%u"
            " output_length=%d output_checked=%zu output_undefined=%zu output_query=%u result=%d\n",
            int_input_length, str_input.int_checked, str_input.int_undefined, str_input.int_query,
            int_output_length, str_output.int_checked, str_output.int_undefined, str_output.int_query, int_result);
    }
    errno = int_result_errno;
    return int_result;
}
