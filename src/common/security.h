/**
 * @file
 * @brief Interfaces for secure DDS participant creation and client trace filtering.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
#ifndef AST_SECURITY_H
#define AST_SECURITY_H
#include "common/protocol.h"
/** @brief Create a DDS participant using the selected role and security credentials. */
dds_entity_t secure_participant(const Options *str_options, bool b_server);
/** @brief Enable or disable the client's DDS trace filter. */
void client_trace_sink(bool b_install);
#endif
