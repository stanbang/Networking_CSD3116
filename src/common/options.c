/* Strict finite CLI parsing. Author: <team>, Codex assistance. Date: 2026-11-03. */
#include <errno.h>
#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "common/protocol.h"

/* Parse bounded positive integers without narrowing or trailing garbage. */
static bool number(const char *str_text, unsigned int *int_out, unsigned int int_max)
{
    char *str_end = NULL;
    errno = 0;
    unsigned long int_value = strtoul(str_text, &str_end, 10);
    if (errno || !*str_text || *str_end || str_text[0] == '-' || !int_value || int_value > int_max) return false;
    *int_out = (unsigned int)int_value;
    return true;
}

/* Parse before threading; argv owns the constant strings throughout execution. */
bool options_parse(int int_argc, char **arr_argv, Options *str_options, bool b_server)
{
    *str_options = (Options){.int_identity = 1, .dbl_duration = 60.0,
                            .str_bot = "idle", .str_fault = "none", .int_credentials_fd = -1};
    for (int int_i = 1; int_i < int_argc; ++int_i) {
        const char *str_arg = arr_argv[int_i];
        if (!b_server && strcmp(str_arg, "--headless") == 0) { str_options->b_headless = true; continue; }
        if (!b_server && strcmp(str_arg, "--register") == 0) { str_options->b_register = true; continue; }
        if (!b_server && strcmp(str_arg, "--remember") == 0) { str_options->b_remember = true; continue; }
        if (!b_server && strcmp(str_arg, "--account-only") == 0) { str_options->b_account_only = true; continue; }
        if (++int_i == int_argc) return false;
        const char *str_value = arr_argv[int_i];
        if (strcmp(str_arg, "--security-dir") == 0) str_options->str_security_dir = str_value;
        else if (b_server && strcmp(str_arg, "--account-dir") == 0) str_options->str_account_dir = str_value;
        else if (!b_server && strcmp(str_arg, "--profile-dir") == 0) str_options->str_profile_dir = str_value;
        else if (!b_server && strcmp(str_arg, "--credentials-fd") == 0) {
            unsigned int int_fd = 0;
            if (strcmp(str_value, "0") != 0 && !number(str_value, &int_fd, INT_MAX)) return false;
            str_options->int_credentials_fd = (int)int_fd;
        } else if (!b_server && strcmp(str_arg, "--account-action") == 0) {
            if (strcmp(str_value, "leaderboard") && strcmp(str_value, "logout") && strcmp(str_value, "forget")) return false;
            str_options->str_account_action = str_value;
            str_options->b_account_only = true;
        }
        else if (b_server && strcmp(str_arg,"--test-scenario") == 0) {
            if(strcmp(str_value,"gameover") != 0) return false;
            str_options->b_test_gameover = true;
        }
        else if (b_server && strcmp(str_arg, "--audit-file") == 0) str_options->str_audit_file = str_value;
        else if (strcmp(str_arg, "--duration") == 0) {
            char *str_end = NULL;
            errno = 0;
            str_options->dbl_duration = strtod(str_value, &str_end);
            if (errno || !*str_value || *str_end || !isfinite(str_options->dbl_duration) ||
                str_options->dbl_duration <= 0 || str_options->dbl_duration > AST_MAX_DURATION) return false;
        } else if (b_server && strcmp(str_arg, "--ticks") == 0) {
            if (!number(str_value, &str_options->int_ticks, AST_HZ * AST_MAX_DURATION)) return false;
        } else if (!b_server && strcmp(str_arg, "--identity") == 0) {
            if (!number(str_value, &str_options->int_identity, AST_IDENTITIES)) return false;
        } else if (!b_server && strcmp(str_arg, "--bot") == 0) {
            if (strcmp(str_value, "idle") && strcmp(str_value, "circle") && strcmp(str_value,"hunter") && strcmp(str_value,"straight")) return false;
            str_options->str_bot = str_value;
        } else if (!b_server && strcmp(str_arg, "--fault") == 0) {
            if (strcmp(str_value,"spoof") && strcmp(str_value,"replay") && strcmp(str_value,"session") &&
                strcmp(str_value,"id-zero") && strcmp(str_value,"id-high") && strcmp(str_value,"wrong-ca") &&
                strcmp(str_value,"no-cert") && strcmp(str_value,"rogue-server") && strcmp(str_value,"flood") &&
                strcmp(str_value,"join-flood") && strcmp(str_value,"slow-reader") && strcmp(str_value,"leave-flood")) return false;
            str_options->str_fault = str_value;
        } else return false;
    }
    return (!str_options->b_register || str_options->int_credentials_fd >= 0) &&
           (!str_options->b_account_only || str_options->b_headless);
}
