/**
 * @file
 * @brief Account authority, storage-worker and authenticated game admission APIs.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
#ifndef AST_SERVER_ACCOUNTS_H
#define AST_SERVER_ACCOUNTS_H

#include "common/account_protocol.h"
#include "common/audit_log.h"
#include "common/event_queue.h"
#include "server/account_store.h"

#define ACCOUNT_EVENT_TAG AST_ACCOUNT_DONE

typedef struct Accounts Accounts;

/**
 * @brief Start the account storage worker and wait for database initialization.
 *
 * Creation waits for the sole storage worker to open its private database.
 */
Accounts *accounts_create(const char *str_data_dir, Audit *str_audit);
/**
 * @brief Set the network event queue used to signal completed account work.
 *
 * Detach events before destroying the queue. Destroy after submitting final scores.
 */
void accounts_set_events(Accounts *str_accounts, EventQueue *str_events);
/** @brief Stop the account storage worker and release the account service. */
bool accounts_destroy(Accounts *str_accounts);
/** @brief Report whether the account service encountered a fatal failure. */
bool accounts_failed(Accounts *str_accounts);

/**
 * @brief Validate and dispatch a request from a certified client identity.
 *
 * Only the DDS worker calls these. A release request refers to this identity.
 */
bool accounts_receive(Accounts *str_accounts, unsigned int int_identity,
                      const AstAccount_Request *str_request, bool *b_release_game);
/** @brief Retrieve the next completed account response for the DDS worker. */
bool accounts_next_response(Accounts *str_accounts, unsigned int *int_identity,
                            AstAccount_Response *str_response);

/**
 * @brief Resolve an authenticated identity and nonce to an account ID.
 *
 * Game hooks may hold the game mutex. The account worker never acquires it.
 */
bool accounts_authorize_join(Accounts *str_accounts, unsigned int int_identity,
                             const uint8_t arr_nonce[16], uint64_t *int_account_id);
/** @brief Mark an authenticated account flow as attached to a game slot. */
bool accounts_game_joined(Accounts *str_accounts, unsigned int int_identity,
                          const uint8_t arr_nonce[16], uint64_t int_account_id);
/** @brief Retire the account flow associated with the departing player. */
void accounts_game_left(Accounts *str_accounts, unsigned int int_identity,
                        const uint8_t arr_nonce[16]);
/** @brief Queue a score for persistence by the storage worker. */
bool accounts_submit_score(Accounts *str_accounts, unsigned int int_identity,
                           const AccountScore *str_score);

#endif
