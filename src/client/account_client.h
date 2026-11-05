/**
 * @file
 * @brief Account protocol state and operations owned by the client DDS worker.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
#ifndef AST_CLIENT_ACCOUNT_H
#define AST_CLIENT_ACCOUNT_H
#include "client/client.h"
#include "client/profile.h"

typedef struct {
    ClientProfile str_profile;
    RememberedUser str_saved;
    LoginCommand str_next;
    AstAccount_Request str_request;
    unsigned char arr_nonce[16], arr_epoch[16], arr_challenge[16], arr_session[16];
    uint64_t int_seq, int_response_seq, int_deadline, int_next_send, int_keepalive, int_challenge_until;
    bool b_pending, b_authenticated, b_profile_loaded, b_started, b_reset_game;
    bool b_profile_revoke, b_end_after_result, b_native_fault;
} ClientAccount;

/** @brief Initialize worker-owned account state for a client. */
void client_account_init(ClientAccount *str_account, Client *str_client);
/** @brief Advance pending account operations and publish requests when due. */
bool client_account_tick(ClientAccount *str_account, Client *str_client, dds_entity_t int_writer, bool b_playing);
/** @brief Validate and handle a received DDS account response. */
bool client_account_receive(ClientAccount *str_account, Client *str_client, const AstAccount_Response *str_response);
/** @brief Check whether the client is ready to request gameplay admission. */
bool client_account_play(const ClientAccount *str_account, const Client *str_client);
/** @brief Publish any required account request during shutdown. */
bool client_account_close(ClientAccount *str_account, dds_entity_t int_writer);
/** @brief Clear the client's account state and credential buffers. */
void client_account_fini(ClientAccount *str_account);
#endif
