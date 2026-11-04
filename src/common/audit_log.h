/* Hash-chained event log API. Author: <team>, Codex assistance. Date: 2026-11-03. */
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
bool audit_open(Audit *str_audit, const char *str_path);
void audit_event(Audit *str_audit, unsigned int int_identity, const char *str_event, const char *str_reason);
bool audit_close(Audit *str_audit);
void audit_dds_install(Audit *str_audit);
void audit_dds_uninstall(void);
#endif
