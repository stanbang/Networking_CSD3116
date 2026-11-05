/* Account protocol state owned by the client DDS worker. Team/Codex, 2026-11-05. */
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

void client_account_init(ClientAccount *str_account, Client *str_client);
bool client_account_tick(ClientAccount *str_account, Client *str_client, dds_entity_t int_writer, bool b_playing);
bool client_account_receive(ClientAccount *str_account, Client *str_client, const AstAccount_Response *str_response);
bool client_account_play(const ClientAccount *str_account, const Client *str_client);
bool client_account_close(ClientAccount *str_account, dds_entity_t int_writer);
void client_account_fini(ClientAccount *str_account);
#endif
