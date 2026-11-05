/* Private remembered-user storage. CSD3116 team with Codex, 2026-11-05. */
#ifndef AST_CLIENT_PROFILE_H
#define AST_CLIENT_PROFILE_H
#include "common/protocol.h"

typedef struct {
    int int_directory, int_lock;
    char str_file[96];
} ClientProfile;

typedef struct {
    unsigned char int_name_length;
    unsigned char arr_name[24];
    unsigned char arr_token[32];
} RememberedUser;

bool profile_open(ClientProfile *str_profile, const Options *str_options);
/* Return 1 for loaded, 0 for absent, -1 for unsafe/corrupt. */
int profile_load(const ClientProfile *str_profile, RememberedUser *str_user);
bool profile_save(const ClientProfile *str_profile, const RememberedUser *str_user);
bool profile_forget(const ClientProfile *str_profile);
void profile_close(ClientProfile *str_profile);
#endif
