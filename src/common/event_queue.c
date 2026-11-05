/**
 * @file
 * @brief Coalesces DDS listener notifications and wakes the network worker.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "common/event_queue.h"
#include "common/protocol.h"

_Static_assert(AST_EVENT_CHANNELS <= 32u, "event mask must hold every reader");

/* A failed rollback cannot leave a live synchronization object on a dead stack. */
static void cleanup_required(int int_result)
{
    if (int_result) { fputs("Event queue synchronization cleanup failed\n", stderr); _Exit(EXIT_FAILURE); }
}

/* Initialize before any DDS listener can observe the queue. */
bool event_queue_init(EventQueue *str_queue)
{
    if (!str_queue) return false;
    *str_queue = (EventQueue){0};
    atomic_init(&str_queue->b_failed, false);
    if (pthread_mutex_init(&str_queue->str_mutex, NULL) != 0) return false;
    pthread_condattr_t str_attr;
    if (pthread_condattr_init(&str_attr) != 0) {
        cleanup_required(pthread_mutex_destroy(&str_queue->str_mutex)); return false;
    }
    int int_result = pthread_condattr_setclock(&str_attr, CLOCK_MONOTONIC);
    bool b_cond = false;
    if (!int_result) {
        int_result = pthread_cond_init(&str_queue->str_cond, &str_attr);
        b_cond = int_result == 0;
    }
    int int_attr_result = pthread_condattr_destroy(&str_attr);
    if (int_result || int_attr_result) {
        if (b_cond) cleanup_required(pthread_cond_destroy(&str_queue->str_cond));
        cleanup_required(pthread_mutex_destroy(&str_queue->str_mutex)); return false;
    }
    str_queue->b_initialized = true;
    return true;
}

/* Callback and worker requeue share a fixed coalescing mask, never a growing list. */
bool event_queue_mark(EventQueue *str_queue, unsigned int int_tag)
{
    if (!str_queue || !str_queue->b_initialized || int_tag >= AST_EVENT_CHANNELS) return false;
    if (pthread_mutex_lock(&str_queue->str_mutex) != 0) {
        atomic_store(&str_queue->b_failed, true); return false;
    }
    str_queue->int_pending |= UINT32_C(1) << int_tag;
    int int_signal = pthread_cond_signal(&str_queue->str_cond);
    int int_unlock = pthread_mutex_unlock(&str_queue->str_mutex);
    if (int_signal || int_unlock) atomic_store(&str_queue->b_failed, true);
    return !int_signal && !int_unlock;
}

/* A single absolute monotonic deadline also bounds repeated spurious wakeups. */
bool event_queue_wait(EventQueue *str_queue, uint32_t *int_mask)
{
    if (!str_queue || !str_queue->b_initialized || !int_mask) return false;
    *int_mask = 0;
    struct timespec str_deadline;
    if (clock_gettime(CLOCK_MONOTONIC, &str_deadline) != 0) return false;
    str_deadline.tv_sec += (time_t)(AST_EVENT_WAIT_NS / UINT64_C(1000000000));
    str_deadline.tv_nsec += (long)(AST_EVENT_WAIT_NS % UINT64_C(1000000000));
    if (str_deadline.tv_nsec >= 1000000000L) { ++str_deadline.tv_sec; str_deadline.tv_nsec -= 1000000000L; }
    if (pthread_mutex_lock(&str_queue->str_mutex) != 0) return false;
    while (!str_queue->int_pending && !atomic_load(&str_queue->b_failed)) {
        int int_result = pthread_cond_timedwait(&str_queue->str_cond, &str_queue->str_mutex, &str_deadline);
        if (int_result == ETIMEDOUT) break;
        if (int_result) { atomic_store(&str_queue->b_failed, true); break; }
    }
    *int_mask = str_queue->int_pending;
    str_queue->int_pending = 0;
    bool b_ok = !atomic_load(&str_queue->b_failed);
    return pthread_mutex_unlock(&str_queue->str_mutex) == 0 && b_ok;
}

/* No callbacks/waiters may remain; a failure requires caller fail-closed exit. */
bool event_queue_fini(EventQueue *str_queue)
{
    if (!str_queue || !str_queue->b_initialized) return false;
    if (pthread_cond_destroy(&str_queue->str_cond) != 0) return false;
    if (pthread_mutex_destroy(&str_queue->str_mutex) != 0) return false;
    str_queue->b_initialized = false;
    return true;
}

/* Initialize every context before creating/attaching a reader. */
bool event_reader_init(EventReader *str_reader, EventQueue *str_queue, unsigned int int_tag)
{
    if (!str_reader || !str_queue || !str_queue->b_initialized || int_tag >= AST_EVENT_CHANNELS) return false;
    *str_reader = (EventReader){.ptr_queue = str_queue, .int_tag = int_tag};
    return true;
}

/* Native callback does no DDS, crypto, audit, allocation or game-state work. */
static void data_available(dds_entity_t int_reader, void *ptr_context)
{
    (void)int_reader;
    EventReader *str_context = ptr_context;
    (void)event_queue_mark(str_context->ptr_queue, str_context->int_tag);
}

/* Set-listener copies callbacks; the stable argument remains caller-owned.
 * Installed API may invoke immediately, so context/queue already exist. */
bool event_reader_attach(EventReader *str_reader, dds_entity_t int_reader)
{
    if (!str_reader || !str_reader->ptr_queue || int_reader <= 0) return false;
    dds_listener_t *str_listener = dds_create_listener(str_reader);
    if (!str_listener) return false;
    bool b_ok = dds_ok(dds_lset_data_available_arg(str_listener, data_available, str_reader, true),
                       "listener_data_available");
    if (b_ok) {
        str_reader->int_reader = int_reader;
        b_ok = dds_ok(dds_set_listener(int_reader, str_listener), "reader_set_listener");
    }
    dds_delete_listener(str_listener);
    return b_ok;
}

/* Called outside queue/game locks, before deleting the owning participant. */
bool event_reader_detach(EventReader *str_reader)
{
    if (!str_reader || str_reader->int_reader <= 0) return true;
    if (!dds_ok(dds_set_listener(str_reader->int_reader, NULL), "reader_remove_listener")) return false;
    str_reader->int_reader = 0;
    return true;
}
