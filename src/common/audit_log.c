/* Public event metadata only; no keys/tokens/raw network strings. Author: <team>, Codex assistance. Date: 2026-11-03. */
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <openssl/evp.h>
#include <dds/ddsrt/log.h>
#include "common/audit_log.h"

/* Fixed output length is known to callers; lowercase canonical hex. */
static void hex_hash(const uint8_t arr_hash[32], char str_hex[65])
{
    const char str_digits[] = "0123456789abcdef";
    for (unsigned int int_i = 0; int_i < 32; ++int_i) {
        str_hex[int_i * 2] = str_digits[arr_hash[int_i] >> 4];
        str_hex[int_i * 2 + 1] = str_digits[arr_hash[int_i] & 15];
    }
    str_hex[64] = '\0';
}

/* Open once before DDS threads; no log is reused across executions. */
bool audit_open(Audit *str_audit, const char *str_path)
{
    *str_audit = (Audit){0};
    int int_result = pthread_mutex_init(&str_audit->str_mutex, NULL);
    if (int_result) return false;
    str_audit->str_file = fopen(str_path, "w");
    if (!str_audit->str_file) { pthread_mutex_destroy(&str_audit->str_mutex); return false; }
    return true;
}

/* Only caller-owned constant event/reason strings are accepted by convention.
 * Independent lock avoids DDS callback recursion through the game-state mutex. */
void audit_event(Audit *str_audit, unsigned int int_identity, const char *str_event, const char *str_reason)
{
    char str_previous[65], str_hash[65], str_line[384];
    mutex_lock(&str_audit->str_mutex);
    struct timespec str_wall = {0};
    if (clock_gettime(CLOCK_REALTIME, &str_wall) != 0) {
        atomic_store(&str_audit->b_failed, true); mutex_unlock(&str_audit->str_mutex); return;
    }
    uint64_t int_unix_ns = (uint64_t)str_wall.tv_sec * UINT64_C(1000000000) + (uint64_t)str_wall.tv_nsec;
    hex_hash(str_audit->arr_hash, str_previous);
    int int_length = snprintf(str_line, sizeof(str_line), "seq=%" PRIu64 " ns=%" PRIu64
        " identity=%u event=%s reason=%s previous=%s", str_audit->int_records + 1,
        int_unix_ns, int_identity, str_event, str_reason, str_previous);
    unsigned int int_hash_size = 0;
    bool b_ok = int_length > 0 && (size_t)int_length < sizeof(str_line) &&
        EVP_Digest(str_line, (size_t)int_length, str_audit->arr_hash, &int_hash_size, EVP_sha256(), NULL) == 1 &&
        int_hash_size == sizeof(str_audit->arr_hash);
    if (b_ok) {
        hex_hash(str_audit->arr_hash, str_hash);
        b_ok = fprintf(str_audit->str_file, "%s hash=%s\n", str_line, str_hash) > 0 && fflush(str_audit->str_file) == 0;
    }
    if (b_ok) ++str_audit->int_records;
    else atomic_store(&str_audit->b_failed, true);
    mutex_unlock(&str_audit->str_mutex);
}

/* Called only after native callbacks and worker threads have stopped. */
bool audit_close(Audit *str_audit)
{
    char str_hash[65];
    hex_hash(str_audit->arr_hash, str_hash);
    printf("AUDIT_ROOT hash=%s records=%" PRIu64 "\n", str_hash, str_audit->int_records);
    bool b_ok = fclose(str_audit->str_file) == 0 && !atomic_load(&str_audit->b_failed);
    if (pthread_mutex_destroy(&str_audit->str_mutex) != 0) b_ok = false;
    return b_ok;
}

/* Middleware messages stay out of the audit text to prevent secrets/address leaks.
 * A security warning is not counted as an authentication failure without evidence. */
static void middleware_log(void *ptr_audit, const dds_log_data_t *str_data)
{
    Audit *str_audit = ptr_audit;
    const char *str_message = str_data->message;
    atomic_fetch_add(&str_audit->int_middleware_events, 1);
    bool b_auth = strstr(str_message, "Handshake") || strstr(str_message, "handshake") ||
        strstr(str_message, "validate_remote_identity") || strstr(str_message, "certificate verify failed") ||
        strstr(str_message, "Certificate not valid");
    bool b_decode = strstr(str_message, "decoding rtps message") || strstr(str_message, "decode_") || strstr(str_message, "decryptionfailed") ||
        strstr(str_message, "MAC") || strstr(str_message, "GCM");
    if (b_auth) atomic_fetch_add(&str_audit->int_auth_failures, 1);
    unsigned int int_kind = b_auth ? 0 : b_decode ? 1 : 2;
    uint64_t int_now = monotonic_ns();
    uint_fast64_t int_previous = atomic_load(&str_audit->arr_last_diagnostic[int_kind]);
    /* A different callback may publish a newer sample before this load. */
    if (int_now < int_previous || int_now - int_previous < UINT64_C(100000000) ||
        !atomic_compare_exchange_strong(&str_audit->arr_last_diagnostic[int_kind], &int_previous, int_now)) return;
    const char *str_reason = b_auth ? "authentication" : b_decode ? "decode" : "middleware";
    audit_event(str_audit, 0, b_auth ? "DDS_AUTH_WARNING" : b_decode ? "DDS_DECODE_WARNING" : "DDS_WARNING", str_reason);
    fprintf(stderr, "DDS_DIAGNOSTIC category=%u reason=%s function=%.80s cause=%s\n", str_data->priority, str_reason,
            str_data->function ? str_data->function : "unknown",
            strstr(str_message, "EVP_DecryptFinal_ex") ? "gcm-final-failed" : "classified");
}

/* 0.10.4 handle_spdp_alive labels a participant secure only when both its secure
 * announcer and identity token are present. This is discovery evidence, not a
 * failed certificate check: unauthenticated peers have no trusted player id. */
static void unauthenticated_discovery(Audit *str_audit)
{
    uint64_t int_now = monotonic_ns();
    uint_fast64_t int_previous = atomic_load(&str_audit->arr_last_diagnostic[3]);
    /* Do not underflow or replace a newer callback's throttle timestamp. */
    if (int_now < int_previous || int_now - int_previous < UINT64_C(100000000) ||
        !atomic_compare_exchange_strong(&str_audit->arr_last_diagnostic[3], &int_previous, int_now)) return;
    atomic_fetch_add(&str_audit->int_middleware_events, 1);
    audit_event(str_audit, 0, "DDS_UNAUTHENTICATED_DISCOVERY", "nonsecure-spdp");
    fputs("DDS_DIAGNOSTIC reason=unauthenticated-discovery function=handle_spdp_alive cause=nonsecure-spdp\n", stderr);
}

/* The installed0.10.4 receiver reports authenticated-decrypt failures and SPDP
 * security capability only at trace level. Other trace bytes stay private. */
static void middleware_trace(void *ptr_audit, const dds_log_data_t *str_data)
{
    if (str_data->function && strcmp(str_data->function, "handle_spdp_alive") == 0 &&
        strstr(str_data->message, "SPDP ST0 ") && strstr(str_data->message, " NEW") &&
        !strstr(str_data->message, " (secure) NEW"))
        unauthenticated_discovery(ptr_audit);
    else if (strstr(str_data->message, "decoding rtps message") && strstr(str_data->message, "failed:"))
        middleware_log(ptr_audit, str_data);
}

/* Synchronous sink registration lets audit lifetime outlive every DDS callback. */
void audit_dds_install(Audit *str_audit)
{
    dds_set_log_sink(middleware_log, str_audit);
    dds_set_trace_sink(middleware_trace, str_audit);
}
/* Restore default sink before closing the file/mutex. */
void audit_dds_uninstall(void)
{
    dds_set_trace_sink(NULL, NULL);
    dds_set_log_sink(NULL, NULL);
}
