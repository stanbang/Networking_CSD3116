/* Observable event-queue behavior regressions. CSD3116 team + AI, 2026-11-04. */
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <time.h>
#include "common/event_queue.h"

/* Monotonic elapsed time makes an untriggered deadline test observable. */
static uint64_t now_ns(void)
{
    struct timespec str_now;
    assert(clock_gettime(CLOCK_MONOTONIC, &str_now) == 0);
    return (uint64_t)str_now.tv_sec * UINT64_C(1000000000) + (uint64_t)str_now.tv_nsec;
}

/* Real asynchronous producer exercises signaling rather than prefilled polling. */
static void *producer(void *ptr_queue)
{
    EventQueue *str_queue = ptr_queue;
    struct timespec str_delay = {.tv_nsec = 10000000};
    while (nanosleep(&str_delay, &str_delay) != 0) assert(errno == EINTR);
    assert(event_queue_mark(str_queue, 4));
    return NULL;
}

/* Repeated notifications coalesce, and invalid shifts cannot alter a valid bit. */
static void queued_cases(EventQueue *str_queue)
{
    uint32_t int_mask = 0;
    assert(event_queue_mark(str_queue, 1));
    assert(event_queue_mark(str_queue, 1));
    assert(event_queue_mark(str_queue, AST_EVENT_CHANNELS - 1));
    assert(!event_queue_mark(str_queue, AST_EVENT_CHANNELS));
    assert(!event_queue_mark(str_queue, UINT32_MAX));
    assert(!event_queue_mark(NULL, 0));
    assert(event_queue_wait(str_queue, &int_mask));
    assert(int_mask == ((UINT32_C(1) << 1) | (UINT32_C(1) << (AST_EVENT_CHANNELS - 1))));
    assert(event_queue_mark(str_queue, 7)); /* a full DDS batch uses this same requeue path */
    assert(event_queue_wait(str_queue, &int_mask));
    assert(int_mask == (UINT32_C(1) << 7));
    puts("PASS queued wake, coalescing, bounded tags and explicit requeue");
}

/* A no-event wait must time out with no bits, not return stale queued work. */
static void timeout_case(EventQueue *str_queue)
{
    uint32_t int_mask = UINT32_MAX;
    uint64_t int_before = now_ns();
    assert(event_queue_wait(str_queue, &int_mask));
    uint64_t int_elapsed = now_ns() - int_before;
    assert(int_mask == 0);
    assert(int_elapsed >= AST_EVENT_WAIT_NS * 4 / 5);
    assert(int_elapsed < UINT64_C(500000000));
    printf("PASS no-event monotonic timeout elapsed_ns=%" PRIu64 "\n", int_elapsed);
}

/* Join the producer before queue finalization, matching callback lifetime rules. */
int main(void)
{
    EventQueue str_queue;
    assert(!event_queue_init(NULL));
    assert(event_queue_init(&str_queue));
    EventReader str_context;
    assert(!event_reader_init(&str_context, &str_queue, AST_EVENT_CHANNELS));
    assert(event_reader_init(&str_context, &str_queue, 0));
    assert(event_reader_detach(&str_context));
    queued_cases(&str_queue);
    timeout_case(&str_queue);
    pthread_t str_thread;
    assert(pthread_create(&str_thread, NULL, producer, &str_queue) == 0);
    uint32_t int_mask = 0;
    assert(event_queue_wait(&str_queue, &int_mask));
    assert(int_mask == (UINT32_C(1) << 4));
    assert(pthread_join(str_thread, NULL) == 0);
    assert(event_queue_fini(&str_queue));
    puts("PASS asynchronous producer wake and safe queue teardown");
    return 0;
}
