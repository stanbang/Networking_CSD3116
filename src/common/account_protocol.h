/**
 * @file
 * @brief Shared limits and validation functions for the account protocol.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
#ifndef AST_ACCOUNT_PROTOCOL_H
#define AST_ACCOUNT_PROTOCOL_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "accounts.h"

/** @brief Check whether every byte in the supplied region is zero. */
bool account_zero(const void *ptr_bytes, size_t int_length);
/** @brief Check the username length and allowed characters. */
bool account_username_valid(const uint8_t *arr_name, size_t int_length);
/** @brief Check the password length and allowed characters. */
bool account_password_valid(const uint8_t *arr_password, size_t int_length);
/** @brief Validate the fields required by the requested account operation. */
bool account_request_valid(const AstAccount_Request *str_request);
/** @brief Validate account response status, fields and bounds. */
bool account_response_valid(const AstAccount_Response *str_response);
#endif
