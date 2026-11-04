/* Diagnostic-only DDS operation markers. CSD3116 team + AI, 2026-11-04.
 * Inject only into the DDS-linked server/client, never the harness or helpers.
 * No policies, wire bytes, keys, entity values or payloads are inspected/changed.
 */
#define _GNU_SOURCE
#include <dds/dds.h>
#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <time.h>

typedef dds_return_t (*WriteCall)(dds_entity_t, const void *);
typedef dds_return_t (*WaitCall)(dds_entity_t, dds_attach_t *, size_t, dds_duration_t);
typedef dds_return_t (*TakeCall)(dds_entity_t, void **, dds_sample_info_t *, size_t, uint32_t);
typedef dds_return_t (*LoanCall)(dds_entity_t, void **, int32_t);
typedef dds_return_t (*DeleteCall)(dds_entity_t);
typedef dds_return_t (*AckCall)(dds_entity_t, dds_duration_t);
static WriteCall str_real_write;
static WaitCall str_real_wait;
static TakeCall str_real_take;
static LoanCall str_real_loan;
static DeleteCall str_real_delete;
static AckCall str_real_acks;

typedef struct {
    char arr_name[16];
    uint64_t int_started;
    bool b_restore;
} Marker;

/* Resolve before application threads; memcpy avoids function-pointer aliasing. */
#define RESOLVE(target, name) do { \
    void *ptr_symbol = dlsym(RTLD_NEXT, name); \
    _Static_assert(sizeof(target) == sizeof(ptr_symbol), "unsupported pointer ABI"); \
    if (!ptr_symbol) { fputs("DDS_MARKER_INIT_FAILED\n", stderr); _Exit(127); } \
    memcpy(&(target), &ptr_symbol, sizeof(target)); \
} while (0)

/* A missing native symbol aborts the diagnostic run rather than changing semantics. */
__attribute__((constructor)) static void markers_init(void)
{
    RESOLVE(str_real_write, "dds_write");
    RESOLVE(str_real_wait, "dds_waitset_wait");
    RESOLVE(str_real_take, "dds_take");
    RESOLVE(str_real_loan, "dds_return_loan");
    RESOLVE(str_real_delete, "dds_delete");
    RESOLVE(str_real_acks, "dds_wait_for_acks");
}

/* Elapsed monotonic nanoseconds contain no process address or security material. */
static uint64_t now_ns(void)
{
    struct timespec str_now = {0};
    if (clock_gettime(CLOCK_MONOTONIC, &str_now) != 0) return 0;
    return (uint64_t)str_now.tv_sec * UINT64_C(1000000000) + (uint64_t)str_now.tv_nsec;
}

/* Kernel thread labels make blocked calls visible without per-call log output. */
static Marker mark_begin(const char *str_operation)
{
    Marker str_mark = {.int_started = now_ns()};
    str_mark.b_restore = prctl(PR_GET_NAME, str_mark.arr_name, 0UL, 0UL, 0UL) == 0;
    if (str_mark.b_restore) (void)prctl(PR_SET_NAME, str_operation, 0UL, 0UL, 0UL);
    return str_mark;
}

/* Nested DDS calls restore the previous marker; only completed slow calls log. */
static void mark_end(const Marker *str_mark, const char *str_operation)
{
    uint64_t int_finished = now_ns();
    if (str_mark->b_restore) (void)prctl(PR_SET_NAME, str_mark->arr_name, 0UL, 0UL, 0UL);
    if (str_mark->int_started && int_finished >= str_mark->int_started &&
        int_finished - str_mark->int_started > UINT64_C(200000000))
        fprintf(stderr, "DDS_SLOW_CALL op=%s elapsed_ns=%" PRIu64 "\n",
                str_operation, int_finished - str_mark->int_started);
}

/* Independent wall-clock readings allow observed drift to be tested, not assumed. */
typedef struct {
    int64_t int_rt, int_dds;
} WallClocks;

static WallClocks wall_now(void)
{
    struct timespec str_rt = {0};
    int64_t int_rt = clock_gettime(CLOCK_REALTIME, &str_rt) == 0 ?
        (int64_t)str_rt.tv_sec * INT64_C(1000000000) + str_rt.tv_nsec : 0;
    return (WallClocks){.int_rt = int_rt, .int_dds = dds_time()};
}

/* Completed waits over200ms and native failures log; no wire data is inspected. */
static void mark_timed_end(const Marker *str_mark, const char *str_operation, dds_duration_t int_requested,
                          dds_return_t int_result, uint64_t int_finished,
                          WallClocks str_before, WallClocks str_after)
{
    if (str_mark->b_restore) (void)prctl(PR_SET_NAME, str_mark->arr_name, 0UL, 0UL, 0UL);
    if (!str_mark->int_started || int_finished < str_mark->int_started ||
        (int_finished - str_mark->int_started <= UINT64_C(200000000) && int_result >= 0)) return;
    fprintf(stderr, "DDS_SLOW_CALL op=%s elapsed_ns=%" PRIu64
        " requested_ns=%" PRId64 " returned=%" PRId32
        " before_dds_ns=%" PRId64 " after_dds_ns=%" PRId64
        " before_rt_ns=%" PRId64 " after_rt_ns=%" PRId64
        " delta_dds_ns=%" PRId64 " delta_rt_ns=%" PRId64 " clock_valid=%u\n",
        str_operation, int_finished - str_mark->int_started, (int64_t)int_requested, (int32_t)int_result,
        str_before.int_dds, str_after.int_dds, str_before.int_rt, str_after.int_rt,
        str_before.int_dds && str_after.int_dds ? str_after.int_dds - str_before.int_dds : 0,
        str_before.int_rt && str_after.int_rt ? str_after.int_rt - str_before.int_rt : 0,
        (unsigned int)(str_before.int_dds && str_after.int_dds && str_before.int_rt && str_after.int_rt));
}

/* Native arguments/results are forwarded unchanged; no sample is dereferenced. */
dds_return_t dds_write(dds_entity_t int_writer, const void *ptr_sample)
{
    int int_errno_before = errno;
    Marker str_mark = mark_begin("ast.write");
    errno = int_errno_before;
    dds_return_t int_result = str_real_write(int_writer, ptr_sample);
    int int_errno_after = errno;
    mark_end(&str_mark, "write");
    errno = int_errno_after;
    return int_result;
}

/* Preserve the exact native timeout, trigger array, and result. */
dds_return_t dds_waitset_wait(dds_entity_t int_waitset, dds_attach_t *arr_tags,
                              size_t int_capacity, dds_duration_t int_timeout)
{
    int int_errno_before = errno;
    Marker str_mark = mark_begin("ast.wait");
    WallClocks str_before = wall_now();
    errno = int_errno_before;
    dds_return_t int_result = str_real_wait(int_waitset, arr_tags, int_capacity, int_timeout);
    int int_errno_after = errno;
    uint64_t int_finished = now_ns();
    WallClocks str_after = wall_now();
    mark_timed_end(&str_mark, "wait", int_timeout, int_result, int_finished, str_before, str_after);
    errno = int_errno_after;
    return int_result;
}

/* Loan buffers remain owned and handled only by the unmodified native caller. */
dds_return_t dds_take(dds_entity_t int_reader, void **arr_samples, dds_sample_info_t *arr_info,
                     size_t int_capacity, uint32_t int_maximum)
{
    int int_errno_before = errno;
    Marker str_mark = mark_begin("ast.take");
    errno = int_errno_before;
    dds_return_t int_result = str_real_take(int_reader, arr_samples, arr_info, int_capacity, int_maximum);
    int int_errno_after = errno;
    mark_end(&str_mark, "take");
    errno = int_errno_after;
    return int_result;
}

/* Return the same native loan without inspecting its payload. */
dds_return_t dds_return_loan(dds_entity_t int_entity, void **arr_samples, int32_t int_count)
{
    int int_errno_before = errno;
    Marker str_mark = mark_begin("ast.loan");
    errno = int_errno_before;
    dds_return_t int_result = str_real_loan(int_entity, arr_samples, int_count);
    int int_errno_after = errno;
    mark_end(&str_mark, "loan");
    errno = int_errno_after;
    return int_result;
}

/* Teardown stalls can be distinguished from application waitset stalls. */
dds_return_t dds_delete(dds_entity_t int_entity)
{
    int int_errno_before = errno;
    Marker str_mark = mark_begin("ast.delete");
    errno = int_errno_before;
    dds_return_t int_result = str_real_delete(int_entity);
    int int_errno_after = errno;
    mark_end(&str_mark, "delete");
    errno = int_errno_after;
    return int_result;
}

/* A received Leave is not proof that every matched reliable reader ACK arrived. */
dds_return_t dds_wait_for_acks(dds_entity_t int_writer, dds_duration_t int_timeout)
{
    int int_errno_before = errno;
    Marker str_mark = mark_begin("ast.acks");
    WallClocks str_before = wall_now();
    errno = int_errno_before;
    dds_return_t int_result = str_real_acks(int_writer, int_timeout);
    int int_errno_after = errno;
    uint64_t int_finished = now_ns();
    WallClocks str_after = wall_now();
    mark_timed_end(&str_mark, "acks", int_timeout, int_result, int_finished, str_before, str_after);
    errno = int_errno_after;
    return int_result;
}
