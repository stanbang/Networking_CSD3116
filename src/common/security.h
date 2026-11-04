/* Native DDS security setup. Author: <team>, Codex assistance. Date: 2026-11-03. */
#ifndef AST_SECURITY_H
#define AST_SECURITY_H
#include "common/protocol.h"
dds_entity_t secure_participant(const Options *str_options, bool b_server);
void client_trace_sink(bool b_install);
#endif
