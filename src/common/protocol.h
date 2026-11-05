/**
 * @file
 * @brief Shared command-line options, gameplay validation and DDS utility APIs.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
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
    const char *str_account_dir, *str_profile_dir, *str_account_action;
    int int_credentials_fd;
    bool b_register, b_remember, b_account_only;
} Options;
/** @brief Parse command-line options and reject invalid values for the selected role. */
bool options_parse(int int_argc, char **arr_argv, Options *str_options, bool b_server);
/** @brief Validate the wire fields of a gameplay join request. */
bool join_valid(const Ast_Join *str_join);
/** @brief Validate the wire fields of an admission response. */
bool response_valid(const Ast_Response *str_response);
/** @brief Validate the wire fields and player bounds of a gameplay input. */
bool input_valid(const Ast_Input *str_input);
/** @brief Validate the wire fields of a gameplay leave request. */
bool leave_valid(const Ast_Leave *str_leave);
/** @brief Validate snapshot bounds and the encoded game state. */
bool snapshot_valid(const Ast_Snapshot *str_snapshot);
/** @brief Read the monotonic clock in nanoseconds. */
uint64_t monotonic_ns(void);
/** @brief Check whether an input lease has expired without subtracting a future timestamp. */
bool lease_expired(uint64_t int_now, uint64_t int_seen);
/** @brief Sleep for the requested nanosecond interval. */
void sleep_ns(uint64_t int_ns);
/** @brief Fill the destination with cryptographically secure random bytes. */
bool random_bytes(void *ptr_data, size_t int_size);
/** @brief Create a DDS reader or writer using the selected topic and reliability. */
dds_entity_t endpoint(dds_entity_t int_participant, const dds_topic_descriptor_t *str_desc,
                      const char *str_kind, unsigned int int_identity, bool b_writer, bool b_reliable);
/** @brief Check the DDS result and report an error for the named operation. */
bool dds_ok(dds_return_t int_result, const char *str_operation);
/** @brief Acquire the mutex or terminate on a synchronization error. */
void mutex_lock(pthread_mutex_t *str_mutex);
/** @brief Release the mutex or terminate on a synchronization error. */
void mutex_unlock(pthread_mutex_t *str_mutex);
#endif
