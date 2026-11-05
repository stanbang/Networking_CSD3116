/**
 * @file
 * @brief Audit log state and interfaces for recording public event metadata.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
#ifndef AST_AUDIT_H
#define AST_AUDIT_H
#include <stdio.h>
#include "common/protocol.h"
typedef struct {
    pthread_mutex_t str_mutex;
    FILE *str_file;
    uint8_t arr_hash[32];
    uint64_t int_records;
    atomic_uint_fast64_t int_auth_failures, int_middleware_events;
    atomic_uint_fast64_t arr_last_diagnostic[4];
    atomic_bool b_failed;
} Audit;
/** @brief Initialize the audit log and open a new output file when requested. */
bool audit_open(Audit *str_audit, const char *str_path);
/** @brief Record an event using its identity, category and public reason. */
void audit_event(Audit *str_audit, unsigned int int_identity, const char *str_event, const char *str_reason);
/** @brief Finalize the audit chain and close the output file. */
bool audit_close(Audit *str_audit);
/** @brief Install the server's DDS diagnostic filter. */
void audit_dds_install(Audit *str_audit);
/** @brief Remove the server's DDS diagnostic filter. */
void audit_dds_uninstall(void);
#endif
