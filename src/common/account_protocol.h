/* Shared account message validation. Team + AI, 2026-11-05. */
#ifndef AST_ACCOUNT_PROTOCOL_H
#define AST_ACCOUNT_PROTOCOL_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "accounts.h"

bool account_zero(const void *ptr_bytes, size_t int_length);
bool account_username_valid(const uint8_t *arr_name, size_t int_length);
bool account_password_valid(const uint8_t *arr_password, size_t int_length);
bool account_request_valid(const AstAccount_Request *str_request);
bool account_response_valid(const AstAccount_Response *str_response);
#endif
