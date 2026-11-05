/**
 * @file
 * @brief Bounded DDS listener-to-worker notification queue and listener lifecycle API.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
#ifndef AST_EVENT_QUEUE_H
#define AST_EVENT_QUEUE_H
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>
#include <dds/dds.h>
#include "common/config.h"

typedef struct {
    pthread_mutex_t str_mutex;
    pthread_cond_t str_cond;
    uint32_t int_pending;
    atomic_bool b_failed;
    bool b_initialized;
} EventQueue;

/* Addresses remain stable from listener installation through DDS detachment. */
typedef struct {
    EventQueue *ptr_queue;
    unsigned int int_tag;
    dds_entity_t int_reader;
} EventReader;

/** @brief Initialize the queue's synchronization objects and pending state. */
bool event_queue_init(EventQueue *str_queue);
/** @brief Set a pending notification bit and wake the worker. */
bool event_queue_mark(EventQueue *str_queue, unsigned int int_tag);
/** @brief Wait for notifications or the polling deadline and report pending bits. */
bool event_queue_wait(EventQueue *str_queue, uint32_t *int_mask);
/**
 * @brief Destroy the queue after all users have detached.
 *
 * Caller must first detach listeners/delete DDS entities; no waiter may remain.
 */
bool event_queue_fini(EventQueue *str_queue);
/** @brief Initialize the stable listener context for a tagged DDS reader. */
bool event_reader_init(EventReader *str_reader, EventQueue *str_queue, unsigned int int_tag);
/** @brief Register the notification callback on a DDS reader. */
bool event_reader_attach(EventReader *str_reader, dds_entity_t int_reader);
/**
 * @brief Remove the listener before its context or queue is destroyed.
 *
 * DDS0.10.4 removal waits for pending/active listener callbacks; hold no queue lock.
 */
bool event_reader_detach(EventReader *str_reader);
#endif
