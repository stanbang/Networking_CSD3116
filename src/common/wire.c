/**
 * @file
 * @brief Writes typed DDS samples after clearing C structure padding.
 * @details Member initialization can leave padding undefined. Clearing it at
 * the DDS write boundary preserves protocol fields, fault injection and QoS.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
#include <stdbool.h>
#include "common/config.h"
#include "common/wire.h"

#ifndef __has_builtin
#error "Outgoing wire protection requires __builtin_clear_padding support"
#elif !__has_builtin(__builtin_clear_padding)
#error "Outgoing wire protection requires __builtin_clear_padding support"
#endif

/* Clear native padding just before the DDS serialization boundary. */
dds_return_t wire_write_account_request(dds_entity_t int_writer, AstAccount_Request *str_request)
{
    if (!str_request) return DDS_RETCODE_BAD_PARAMETER;
    __builtin_clear_padding(str_request);
    return dds_write(int_writer, str_request);
}

dds_return_t wire_write_account_response(dds_entity_t int_writer, AstAccount_Response *str_response)
{
    if (!str_response) return DDS_RETCODE_BAD_PARAMETER;
    __builtin_clear_padding(str_response);
    return dds_write(int_writer, str_response);
}

/* No validation here: negative tests must still send deliberately invalid fields. */
dds_return_t wire_write_join(dds_entity_t int_writer, Ast_Join *str_join)
{
    if (!str_join) return DDS_RETCODE_BAD_PARAMETER;
    __builtin_clear_padding(str_join);
    return dds_write(int_writer, str_join);
}

/* Responses remain addressed by their original nonce and preserve native errors. */
dds_return_t wire_write_response(dds_entity_t int_writer, Ast_Response *str_response)
{
    if (!str_response) return DDS_RETCODE_BAD_PARAMETER;
    __builtin_clear_padding(str_response);
    return dds_write(int_writer, str_response);
}

/* Clear after replay/spoof mutations, without changing those test field values. */
dds_return_t wire_write_input(dds_entity_t int_writer, Ast_Input *str_input)
{
    if (!str_input) return DDS_RETCODE_BAD_PARAMETER;
    __builtin_clear_padding(str_input);
    return dds_write(int_writer, str_input);
}

/* Keep the reliable Leave write/ACK lifecycle unchanged. */
dds_return_t wire_write_leave(dds_entity_t int_writer, Ast_Leave *str_leave)
{
    if (!str_leave) return DDS_RETCODE_BAD_PARAMETER;
    __builtin_clear_padding(str_leave);
    return dds_write(int_writer, str_leave);
}

/* Check every sequence before touching any backing storage; loops stay bounded. */
static bool sequence_bounds(uint32_t int_length, uint32_t int_maximum,
                            uint32_t int_cap, const void *ptr_buffer)
{
    return int_length <= int_maximum && int_maximum <= int_cap && (!int_length || ptr_buffer);
}

/* This thread owns the local Frame arrays, never a shared state or reader loan. */
dds_return_t wire_write_snapshot(dds_entity_t int_writer, Ast_Snapshot *str_snapshot)
{
    if (!str_snapshot ||
        !sequence_bounds(str_snapshot->arr_ships._length, str_snapshot->arr_ships._maximum,
                         AST_PLAYERS, str_snapshot->arr_ships._buffer) ||
        !sequence_bounds(str_snapshot->arr_bullets._length, str_snapshot->arr_bullets._maximum,
                         AST_BULLETS, str_snapshot->arr_bullets._buffer) ||
        !sequence_bounds(str_snapshot->arr_asteroids._length, str_snapshot->arr_asteroids._maximum,
                         AST_ASTEROIDS, str_snapshot->arr_asteroids._buffer)) return DDS_RETCODE_BAD_PARAMETER;
    for (uint32_t int_i = 0; int_i < str_snapshot->arr_ships._length; ++int_i)
        __builtin_clear_padding(&str_snapshot->arr_ships._buffer[int_i]);
    for (uint32_t int_i = 0; int_i < str_snapshot->arr_bullets._length; ++int_i)
        __builtin_clear_padding(&str_snapshot->arr_bullets._buffer[int_i]);
    for (uint32_t int_i = 0; int_i < str_snapshot->arr_asteroids._length; ++int_i)
        __builtin_clear_padding(&str_snapshot->arr_asteroids._buffer[int_i]);
    __builtin_clear_padding(str_snapshot);
    return dds_write(int_writer, str_snapshot);
}
