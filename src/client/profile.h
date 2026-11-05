/**
 * @file
 * @brief Private client profile and remembered-user storage API.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
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

/** @brief Open the private remembered-user profile for the configured client. */
bool profile_open(ClientProfile *str_profile, const Options *str_options);
/**
 * @brief Load a remembered username and resume token.
 *
 * Return 1 for loaded, 0 for absent, -1 for unsafe/corrupt.
 */
int profile_load(const ClientProfile *str_profile, RememberedUser *str_user);
/** @brief Replace the private profile with the supplied remembered credentials. */
bool profile_save(const ClientProfile *str_profile, const RememberedUser *str_user);
/** @brief Remove any saved credentials for the current profile. */
bool profile_forget(const ClientProfile *str_profile);
/** @brief Release the profile's directory and lock handles. */
void profile_close(ClientProfile *str_profile);
#endif
