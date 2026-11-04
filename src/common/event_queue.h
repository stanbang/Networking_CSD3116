/* Bounded listener-to-worker notification handoff. CSD3116 team + AI, 2026-11-04. */
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

bool event_queue_init(EventQueue *str_queue);
bool event_queue_mark(EventQueue *str_queue, unsigned int int_tag);
bool event_queue_wait(EventQueue *str_queue, uint32_t *int_mask);
/* Caller must first detach listeners/delete DDS entities; no waiter may remain. */
bool event_queue_fini(EventQueue *str_queue);
bool event_reader_init(EventReader *str_reader, EventQueue *str_queue, unsigned int int_tag);
bool event_reader_attach(EventReader *str_reader, dds_entity_t int_reader);
/* DDS0.10.4 removal waits for pending/active listener callbacks; hold no queue lock. */
bool event_reader_detach(EventReader *str_reader);
#endif
