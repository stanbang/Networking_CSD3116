/* Typed outgoing padding boundary. Team + declared Codex assistance, 2026-11-04. */
#ifndef AST_WIRE_H
#define AST_WIRE_H
#include <dds/dds.h>
#include "messages.h"
#include "accounts.h"

/* Caller exclusively owns mutable samples/backing arrays through synchronous DDS
 * write. Clear only padding after final field updates; return native DDS result. */
dds_return_t wire_write_join(dds_entity_t int_writer, Ast_Join *str_join);
dds_return_t wire_write_response(dds_entity_t int_writer, Ast_Response *str_response);
dds_return_t wire_write_input(dds_entity_t int_writer, Ast_Input *str_input);
dds_return_t wire_write_leave(dds_entity_t int_writer, Ast_Leave *str_leave);
dds_return_t wire_write_snapshot(dds_entity_t int_writer, Ast_Snapshot *str_snapshot);
dds_return_t wire_write_account_request(dds_entity_t int_writer, AstAccount_Request *str_request);
dds_return_t wire_write_account_response(dds_entity_t int_writer, AstAccount_Response *str_response);
#endif
