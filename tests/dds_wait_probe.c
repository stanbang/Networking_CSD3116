/* Diagnostic wait-clock comparison, CSD3116 team + AI, 2026-11-04.
 * Creates only an untriggered guard/waitset, no application reader/writer/data.
 * It does not change system clocks, security policies or production code.
 */
#define _POSIX_C_SOURCE 200809L
#include <dds/dds.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define PROBE_WAIT_NS INT64_C(50000000)
typedef struct {
    int64_t int_rt, int_mono, int_raw, int_dds;
} Clocks;

/* Clock values are timestamps only; no process address or security state. */
static Clocks clocks_now(void)
{
    struct timespec str_rt = {0}, str_mono = {0}, str_raw = {0};
    if (clock_gettime(CLOCK_REALTIME, &str_rt) != 0 ||
        clock_gettime(CLOCK_MONOTONIC, &str_mono) != 0 ||
        clock_gettime(CLOCK_MONOTONIC_RAW, &str_raw) != 0) {
        fputs("PROBE_CLOCK_ERROR\n", stderr); exit(1);
    }
    return (Clocks){
        .int_rt = (int64_t)str_rt.tv_sec * INT64_C(1000000000) + str_rt.tv_nsec,
        .int_mono = (int64_t)str_mono.tv_sec * INT64_C(1000000000) + str_mono.tv_nsec,
        .int_raw = (int64_t)str_raw.tv_sec * INT64_C(1000000000) + str_raw.tv_nsec,
        .int_dds = dds_time()};
}

/* Print both endpoints and independent deltas, without assuming clocks agree. */
static void print_sample(const char *str_kind, unsigned int int_index, int int_result,
                         Clocks str_before, Clocks str_after)
{
    printf("WAIT_SAMPLE kind=%s index=%u requested_ns=%" PRId64 " returned=%d"
        " before_rt_ns=%" PRId64 " after_rt_ns=%" PRId64
        " before_mono_ns=%" PRId64 " after_mono_ns=%" PRId64
        " before_dds_ns=%" PRId64 " after_dds_ns=%" PRId64
        " delta_rt_ns=%" PRId64 " delta_mono_ns=%" PRId64
        " delta_raw_ns=%" PRId64 " delta_dds_ns=%" PRId64 "\n",
        str_kind, int_index, PROBE_WAIT_NS, int_result,
        str_before.int_rt, str_after.int_rt, str_before.int_mono, str_after.int_mono,
        str_before.int_dds, str_after.int_dds, str_after.int_rt - str_before.int_rt,
        str_after.int_mono - str_before.int_mono, str_after.int_raw - str_before.int_raw,
        str_after.int_dds - str_before.int_dds);
}

/* Sole waiter and false guard: zero is the expected native timeout result. */
static int sample_dds(dds_entity_t int_waitset, unsigned int int_index)
{
    dds_attach_t int_tag = 0;
    Clocks str_before = clocks_now();
    dds_return_t int_result = dds_waitset_wait(int_waitset, &int_tag, 1, DDS_MSECS(50));
    Clocks str_after = clocks_now();
    print_sample("dds_guard", int_index, int_result, str_before, str_after);
    return int_result == 0 ? 0 : 1;
}

/* Compare against an explicitly monotonic pthread deadline, not DDS internals. */
static int sample_pthread(pthread_cond_t *str_cond, pthread_mutex_t *str_mutex,
                          unsigned int int_index)
{
    if (pthread_mutex_lock(str_mutex) != 0) return 1;
    Clocks str_before = clocks_now();
    int64_t int_deadline = str_before.int_mono + PROBE_WAIT_NS;
    struct timespec str_deadline = {.tv_sec = (time_t)(int_deadline / INT64_C(1000000000)),
        .tv_nsec = (long)(int_deadline % INT64_C(1000000000))};
    int int_result = pthread_cond_timedwait(str_cond, str_mutex, &str_deadline);
    Clocks str_after = clocks_now();
    int int_unlock = pthread_mutex_unlock(str_mutex);
    print_sample("pthread_monotonic", int_index, int_result, str_before, str_after);
    return int_result == ETIMEDOUT && !int_unlock ? 0 : 1;
}

/* Initialize the comparison condition using the installed POSIX clock API. */
static int cond_init_monotonic(pthread_cond_t *str_cond)
{
    pthread_condattr_t str_attr;
    if (pthread_condattr_init(&str_attr) != 0) return 1;
    int int_result = pthread_condattr_setclock(&str_attr, CLOCK_MONOTONIC);
    if (!int_result) int_result = pthread_cond_init(str_cond, &str_attr);
    int int_cleanup = pthread_condattr_destroy(&str_attr);
    return int_result || int_cleanup ? 1 : 0;
}

/* Positional sample count is bounded; alarm bounds even a pathological DDS wait. */
int main(int int_argc, char **arr_argv)
{
    unsigned long int_count = 12;
    if (int_argc > 2) return 2;
    if (int_argc == 2) {
        char *str_end = NULL; errno = 0;
        int_count = strtoul(arr_argv[1], &str_end, 10);
        if (errno || !*arr_argv[1] || *str_end || int_count < 1 || int_count > 100) return 2;
    }
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("WAIT_PROBE samples=%lu requested_dds_ns=%" PRId64 " alarm_seconds=20\n",
           int_count, (int64_t)DDS_MSECS(50));
    alarm(20);
    dds_entity_t int_participant = dds_create_participant(0, NULL, NULL);
    if (int_participant < 0) { fputs("PROBE_PARTICIPANT_ERROR\n", stderr); return 1; }
    dds_entity_t int_waitset = dds_create_waitset(int_participant);
    dds_entity_t int_guard = dds_create_guardcondition(int_participant);
    if (int_waitset < 0 || int_guard < 0 || dds_set_guardcondition(int_guard, false) < 0 ||
        dds_waitset_attach(int_waitset, int_guard, 1) < 0) {
        (void)dds_delete(int_participant); fputs("PROBE_SETUP_ERROR\n", stderr); return 1;
    }
    pthread_mutex_t str_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t str_cond;
    if (cond_init_monotonic(&str_cond)) { (void)dds_delete(int_participant); return 1; }
    int int_failed = 0;
    for (unsigned int int_i = 0; int_i < int_count && !int_failed; ++int_i) {
        int_failed = sample_dds(int_waitset, int_i);
        if (!int_failed) int_failed = sample_pthread(&str_cond, &str_mutex, int_i);
    }
    int_failed |= pthread_cond_destroy(&str_cond) != 0;
    int_failed |= pthread_mutex_destroy(&str_mutex) != 0;
    int_failed |= dds_delete(int_participant) < 0;
    alarm(0);
    printf("WAIT_PROBE_EXIT result=%d\n", int_failed);
    return int_failed;
}
