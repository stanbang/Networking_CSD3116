/**
 * @file
 * @brief Interfaces for CA-scoped account and profile storage paths.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
#ifndef AST_ACCOUNT_PATHS_H
#define AST_ACCOUNT_PATHS_H
#include <stdbool.h>
#include <limits.h>
/** @brief Derive the account storage scope from the trusted CA certificate. */
bool account_scope(const char *str_security_dir, char arr_scope[65]);
/** @brief Resolve and validate the private directory for a type of account state. */
bool account_private_dir(const char *str_security_dir, const char *str_kind, char arr_path[PATH_MAX]);
#endif
