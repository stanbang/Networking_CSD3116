/* Shared APIs. Author: <team>, Codex assistance. Date: 2026-11-03. */
#ifndef AST_PROTOCOL_H
#define AST_PROTOCOL_H
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>
#include <dds/dds.h>
#include "messages.h"
#include "common/config.h"
typedef struct {
    unsigned int int_identity, int_ticks;
    double dbl_duration;
    bool b_headless;
    const char *str_bot, *str_fault;
    const char *str_security_dir, *str_audit_file;
    bool b_test_gameover;
} Options;
bool options_parse(int int_argc, char **arr_argv, Options *str_options, bool b_server);
bool join_valid(const Ast_Join *str_join);
bool response_valid(const Ast_Response *str_response);
bool input_valid(const Ast_Input *str_input);
bool leave_valid(const Ast_Leave *str_leave);
bool snapshot_valid(const Ast_Snapshot *str_snapshot);
uint64_t monotonic_ns(void);
bool lease_expired(uint64_t int_now, uint64_t int_seen);
void sleep_ns(uint64_t int_ns);
bool random_bytes(void *ptr_data, size_t int_size);
dds_entity_t endpoint(dds_entity_t int_participant, const dds_topic_descriptor_t *str_desc,
                      const char *str_kind, unsigned int int_identity, bool b_writer, bool b_reliable);
bool dds_ok(dds_return_t int_result, const char *str_operation);
void mutex_lock(pthread_mutex_t *str_mutex);
void mutex_unlock(pthread_mutex_t *str_mutex);
#endif
