/* CA-scoped private Linux state paths. Team + AI, 2026-11-05. */
#ifndef AST_ACCOUNT_PATHS_H
#define AST_ACCOUNT_PATHS_H
#include <stdbool.h>
#include <limits.h>
bool account_scope(const char *str_security_dir, char arr_scope[65]);
bool account_private_dir(const char *str_security_dir, const char *str_kind, char arr_path[PATH_MAX]);
#endif
