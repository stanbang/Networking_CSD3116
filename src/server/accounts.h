/* Account authority and storage worker. CSD3116 team + Codex, 2026-11-05. */
#ifndef AST_SERVER_ACCOUNTS_H
#define AST_SERVER_ACCOUNTS_H

#include "common/account_protocol.h"
#include "common/audit_log.h"
#include "common/event_queue.h"
#include "server/account_store.h"

#define ACCOUNT_EVENT_TAG AST_ACCOUNT_DONE

typedef struct Accounts Accounts;

/* Creation waits for the sole storage worker to open its private database. */
Accounts *accounts_create(const char *str_data_dir, Audit *str_audit);
/* Detach events before destroying the queue. Destroy after submitting final scores. */
void accounts_set_events(Accounts *str_accounts, EventQueue *str_events);
bool accounts_destroy(Accounts *str_accounts);
bool accounts_failed(Accounts *str_accounts);

/* Only the DDS worker calls these. A release request refers to this identity. */
bool accounts_receive(Accounts *str_accounts, unsigned int int_identity,
                      const AstAccount_Request *str_request, bool *b_release_game);
bool accounts_next_response(Accounts *str_accounts, unsigned int *int_identity,
                            AstAccount_Response *str_response);

/* Game hooks may hold the game mutex. The account worker never acquires it. */
bool accounts_authorize_join(Accounts *str_accounts, unsigned int int_identity,
                             const uint8_t arr_nonce[16], uint64_t *int_account_id);
bool accounts_game_joined(Accounts *str_accounts, unsigned int int_identity,
                          const uint8_t arr_nonce[16], uint64_t int_account_id);
void accounts_game_left(Accounts *str_accounts, unsigned int int_identity,
                        const uint8_t arr_nonce[16]);
bool accounts_submit_score(Accounts *str_accounts, unsigned int int_identity,
                           const AccountScore *str_score);

#endif
