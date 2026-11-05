/**
 * @file
 * @brief Typed DDS writers that clear structure padding before transmission.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
#ifndef AST_WIRE_H
#define AST_WIRE_H
#include <dds/dds.h>
#include "messages.h"
#include "accounts.h"

/**
 * @brief Write a join request through the padding-clearing DDS boundary.
 *
 * Caller exclusively owns mutable samples/backing arrays through synchronous DDS
 * write. Clear only padding after final field updates; return native DDS result.
 */
dds_return_t wire_write_join(dds_entity_t int_writer, Ast_Join *str_join);
/** @brief Write an admission response through the padding-clearing DDS boundary. */
dds_return_t wire_write_response(dds_entity_t int_writer, Ast_Response *str_response);
/** @brief Write player input through the padding-clearing DDS boundary. */
dds_return_t wire_write_input(dds_entity_t int_writer, Ast_Input *str_input);
/** @brief Write a leave request through the padding-clearing DDS boundary. */
dds_return_t wire_write_leave(dds_entity_t int_writer, Ast_Leave *str_leave);
/** @brief Write a snapshot through the padding-clearing DDS boundary. */
dds_return_t wire_write_snapshot(dds_entity_t int_writer, Ast_Snapshot *str_snapshot);
/** @brief Write an account request through the padding-clearing DDS boundary. */
dds_return_t wire_write_account_request(dds_entity_t int_writer, AstAccount_Request *str_request);
/** @brief Write an account response through the padding-clearing DDS boundary. */
dds_return_t wire_write_account_response(dds_entity_t int_writer, AstAccount_Response *str_response);
#endif
