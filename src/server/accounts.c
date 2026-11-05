/**
 * @file
 * @brief Server account flows and asynchronous SQLite and password verification work.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
#include <limits.h>
#include <openssl/crypto.h>
#include <openssl/hmac.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server/accounts.h"

#define ACCOUNT_SECOND UINT64_C(1000000000)
#define ACCOUNT_FLOW_NS (30 * ACCOUNT_SECOND)
#define ACCOUNT_LEASE_NS (10 * ACCOUNT_SECOND)
#define ACCOUNT_JOB_NS (10 * ACCOUNT_SECOND)
#define ACCOUNT_KDF_NS (2 * ACCOUNT_SECOND)
#define ACCOUNT_DIGEST_BYTES 32u

typedef struct {
    uint64_t int_tokens, int_last;
} AccountBucket;

typedef struct {
    AstAccount_Response str_message;
    uint8_t arr_digest[ACCOUNT_DIGEST_BYTES];
    uint64_t int_expiry;
    bool b_send;
} AccountReply;

typedef struct {
    AstAccount_Request str_message;
    uint8_t arr_digest[ACCOUNT_DIGEST_BYTES];
    uint64_t int_generation, int_account_id;
    bool b_queued;
} AccountJob;

typedef enum { FLOW_CLOSED, FLOW_PREAUTH, FLOW_AUTHENTICATED } FlowState;

typedef struct {
    FlowState int_state;
    uint8_t arr_nonce[16], arr_challenge[16], arr_session[16];
    uint8_t arr_pending_digest[ACCOUNT_DIGEST_BYTES];
    uint64_t int_account_id, int_generation, int_highwater, int_response_seq;
    uint64_t int_expiry, int_job_expiry, int_last_kdf, int_last_audit;
    bool b_game, b_pending, b_revoking, b_revoke_failed;
    AccountBucket str_input_budget, str_reply_budget;
    AccountJob str_job, str_revoke;
    AccountReply str_reply, str_transient;
} AccountFlow;

typedef struct {
    AccountScore str_score;
    uint64_t int_version;
    bool b_pending;
} PendingScore;

struct Accounts {
    pthread_mutex_t str_mutex;
    pthread_cond_t str_cond;
    pthread_t str_thread;
    Audit *ptr_audit;
    EventQueue *ptr_events;
    const char *str_data_dir;
    bool b_started, b_open, b_stop, b_score_failed;
    atomic_bool b_failed;
    uint8_t arr_epoch[16], arr_hmac_key[ACCOUNT_DIGEST_BYTES];
    AccountBucket str_kdf_budget, str_reply_budget;
    unsigned int int_next_job, int_next_reply;
    AccountFlow arr_flows[AST_IDENTITIES];
    PendingScore arr_scores[AST_IDENTITIES];
};

static bool identity_valid(unsigned int int_identity)
{
    return int_identity > 0 && int_identity <= AST_IDENTITIES;
}

static void checked_sync(int int_result)
{
    if (int_result != 0) {
        fputs("Account worker synchronization failed\n", stderr);
        _Exit(EXIT_FAILURE);
    }
}

/* All budget arithmetic is bounded by burst * nanoseconds, including after an idle period. */
static void replenish(AccountBucket *str_bucket, uint64_t int_now, unsigned int int_rate,
                      unsigned int int_burst)
{
    uint64_t int_cap = int_burst * ACCOUNT_SECOND;
    if (!str_bucket->int_last) {
        str_bucket->int_tokens = int_cap;
    } else if (int_now >= str_bucket->int_last) {
        uint64_t int_elapsed = int_now - str_bucket->int_last;
        uint64_t int_room = int_cap - str_bucket->int_tokens;
        str_bucket->int_tokens += int_elapsed > int_room / int_rate ? int_room : int_elapsed * int_rate;
    }
    str_bucket->int_last = int_now;
}

static void put_integer(uint8_t **ptr_output, uint64_t int_value, unsigned int int_bytes)
{
    for (unsigned int int_i = int_bytes; int_i > 0; --int_i)
        *(*ptr_output)++ = (uint8_t)(int_value >> ((int_i - 1) * CHAR_BIT));
}

static void put_bytes(uint8_t **ptr_output, const void *ptr_input, size_t int_size)
{
    memcpy(*ptr_output, ptr_input, int_size);
    *ptr_output += int_size;
}

/* The request digest is deterministic across C padding and host byte order. */
static bool request_digest(const Accounts *str_accounts, const AstAccount_Request *str_request,
                           uint8_t arr_digest[ACCOUNT_DIGEST_BYTES])
{
    uint8_t arr_bytes[sizeof(*str_request)];
    uint8_t *ptr_next = arr_bytes;
    put_integer(&ptr_next, str_request->int_version, 2);
    put_integer(&ptr_next, str_request->int_operation, 2);
    put_integer(&ptr_next, str_request->int_seq, 8);
    put_bytes(&ptr_next, str_request->arr_client_nonce, sizeof(str_request->arr_client_nonce));
    put_bytes(&ptr_next, str_request->arr_request_nonce, sizeof(str_request->arr_request_nonce));
    put_bytes(&ptr_next, str_request->arr_server_epoch, sizeof(str_request->arr_server_epoch));
    put_bytes(&ptr_next, str_request->arr_server_challenge, sizeof(str_request->arr_server_challenge));
    put_bytes(&ptr_next, str_request->arr_account_session, sizeof(str_request->arr_account_session));
    put_bytes(&ptr_next, str_request->arr_resume_token, sizeof(str_request->arr_resume_token));
    put_integer(&ptr_next, str_request->int_username_length, 2);
    put_integer(&ptr_next, str_request->int_password_length, 2);
    put_bytes(&ptr_next, str_request->arr_username, sizeof(str_request->arr_username));
    put_bytes(&ptr_next, str_request->arr_password, sizeof(str_request->arr_password));
    *ptr_next++ = str_request->b_remember ? 1 : 0;
    unsigned int int_length = 0;
    bool b_ok = HMAC(EVP_sha256(), str_accounts->arr_hmac_key, ACCOUNT_DIGEST_BYTES,
                     arr_bytes, (size_t)(ptr_next - arr_bytes), arr_digest, &int_length) != NULL &&
                int_length == ACCOUNT_DIGEST_BYTES;
    OPENSSL_cleanse(arr_bytes, sizeof(arr_bytes));
    return b_ok;
}

static bool random_nonzero(void *ptr_value, size_t int_size)
{
    return random_bytes(ptr_value, int_size) && !account_zero(ptr_value, int_size);
}

/* A detach synchronizes through the same mutex before the DDS event queue is freed. */
static void notify_locked(Accounts *str_accounts)
{
    if (str_accounts->ptr_events && !event_queue_mark(str_accounts->ptr_events, ACCOUNT_EVENT_TAG))
        atomic_store(&str_accounts->b_failed, true);
}

static AstAccount_Response response_base(const Accounts *str_accounts, const AstAccount_Request *str_request,
                                        uint16_t int_status)
{
    AstAccount_Response str_reply = {0};
    str_reply.int_version = AstAccount_VERSION;
    str_reply.int_operation = str_request->int_operation;
    str_reply.int_status = int_status;
    str_reply.int_request_seq = str_request->int_seq;
    memcpy(str_reply.arr_client_nonce, str_request->arr_client_nonce, sizeof(str_reply.arr_client_nonce));
    memcpy(str_reply.arr_request_nonce, str_request->arr_request_nonce, sizeof(str_reply.arr_request_nonce));
    memcpy(str_reply.arr_server_epoch, str_accounts->arr_epoch, sizeof(str_reply.arr_server_epoch));
    memcpy(str_reply.arr_server_challenge, str_request->arr_server_challenge, sizeof(str_reply.arr_server_challenge));
    return str_reply;
}

static void save_reply(AccountReply *str_cache, const AstAccount_Response *str_message,
                       const uint8_t arr_digest[ACCOUNT_DIGEST_BYTES], uint64_t int_now)
{
    OPENSSL_cleanse(str_cache, sizeof(*str_cache));
    str_cache->str_message = *str_message;
    if (arr_digest) memcpy(str_cache->arr_digest, arr_digest, ACCOUNT_DIGEST_BYTES);
    str_cache->int_expiry = int_now + ACCOUNT_FLOW_NS;
    str_cache->b_send = true;
}

/* Reserved revocation survives disconnect and blocks reopening this identity. */
static void cancel_pending(AccountFlow *str_flow)
{
    ++str_flow->int_generation;
    str_flow->b_pending = false;
    str_flow->int_job_expiry = 0;
    OPENSSL_cleanse(&str_flow->str_job, sizeof(str_flow->str_job));
    OPENSSL_cleanse(str_flow->arr_pending_digest, sizeof(str_flow->arr_pending_digest));
    OPENSSL_cleanse(&str_flow->str_reply, sizeof(str_flow->str_reply));
    OPENSSL_cleanse(&str_flow->str_transient, sizeof(str_flow->str_transient));
}

static void retire_flow(AccountFlow *str_flow)
{
    cancel_pending(str_flow);
    str_flow->int_state = FLOW_CLOSED;
    str_flow->b_game = false;
    str_flow->int_account_id = 0;
    str_flow->int_highwater = 0;
    str_flow->int_expiry = 0;
    OPENSSL_cleanse(str_flow->arr_nonce, sizeof(str_flow->arr_nonce));
    OPENSSL_cleanse(str_flow->arr_challenge, sizeof(str_flow->arr_challenge));
    OPENSSL_cleanse(str_flow->arr_session, sizeof(str_flow->arr_session));
}

static void expire_flow(AccountFlow *str_flow, uint64_t int_now)
{
    if (str_flow->int_state != FLOW_CLOSED && !str_flow->b_game && int_now >= str_flow->int_expiry)
        retire_flow(str_flow);
    if (str_flow->b_pending && int_now >= str_flow->int_job_expiry) {
        if (str_flow->int_state == FLOW_PREAUTH) retire_flow(str_flow);
        else cancel_pending(str_flow);
    }
    if (str_flow->str_reply.int_expiry && int_now >= str_flow->str_reply.int_expiry)
        OPENSSL_cleanse(&str_flow->str_reply, sizeof(str_flow->str_reply));
    if (str_flow->str_transient.int_expiry && int_now >= str_flow->str_transient.int_expiry)
        OPENSSL_cleanse(&str_flow->str_transient, sizeof(str_flow->str_transient));
}

static bool same_bytes(const void *ptr_left, const void *ptr_right, size_t int_size)
{
    return CRYPTO_memcmp(ptr_left, ptr_right, int_size) == 0;
}

static bool flow_matches(const Accounts *str_accounts, const AccountFlow *str_flow,
                         const AstAccount_Request *str_request)
{
    return str_flow->int_state != FLOW_CLOSED &&
           same_bytes(str_accounts->arr_epoch, str_request->arr_server_epoch, 16) &&
           same_bytes(str_flow->arr_challenge, str_request->arr_server_challenge, 16) &&
           same_bytes(str_flow->arr_nonce, str_request->arr_client_nonce, 16);
}

static bool authenticated(const AccountFlow *str_flow, const AstAccount_Request *str_request)
{
    return str_flow->int_state == FLOW_AUTHENTICATED &&
           same_bytes(str_flow->arr_session, str_request->arr_account_session, 16);
}

static bool is_login(uint16_t int_operation)
{
    return int_operation == AstAccount_REGISTER || int_operation == AstAccount_LOGIN ||
           int_operation == AstAccount_RESUME;
}

static void refresh_lease(AccountFlow *str_flow, uint64_t int_now)
{
    if (str_flow->int_state == FLOW_AUTHENTICATED && !str_flow->b_game)
        str_flow->int_expiry = int_now + ACCOUNT_LEASE_NS;
}

static void queue_job_locked(Accounts *str_accounts, AccountFlow *str_flow,
                             const AstAccount_Request *str_request,
                             const uint8_t arr_digest[ACCOUNT_DIGEST_BYTES], uint64_t int_now)
{
    str_flow->str_job = (AccountJob){.str_message = *str_request, .int_generation = str_flow->int_generation,
                                    .int_account_id = str_flow->int_account_id, .b_queued = true};
    memcpy(str_flow->str_job.arr_digest, arr_digest, ACCOUNT_DIGEST_BYTES);
    memcpy(str_flow->arr_pending_digest, arr_digest, ACCOUNT_DIGEST_BYTES);
    str_flow->int_highwater = str_request->int_seq;
    str_flow->b_pending = true;
    str_flow->int_job_expiry = int_now + ACCOUNT_JOB_NS;
    checked_sync(pthread_cond_signal(&str_accounts->str_cond));
}

static void revoke_locked(Accounts *str_accounts, AccountFlow *str_flow,
                          const AstAccount_Request *str_request,
                          const uint8_t arr_digest[ACCOUNT_DIGEST_BYTES], bool b_close)
{
    uint64_t int_account_id = str_flow->int_account_id;
    if (b_close) retire_flow(str_flow);
    else {
        cancel_pending(str_flow);
        str_flow->int_highwater = str_request->int_seq;
    }
    str_flow->str_revoke = (AccountJob){.str_message = *str_request, .int_generation = str_flow->int_generation,
                                       .int_account_id = int_account_id, .b_queued = true};
    memcpy(str_flow->str_revoke.arr_digest, arr_digest, ACCOUNT_DIGEST_BYTES);
    str_flow->b_revoking = true;
    checked_sync(pthread_cond_signal(&str_accounts->str_cond));
}

static uint16_t reply_status(Accounts *str_accounts, AccountFlow *str_flow,
                         const AstAccount_Request *str_request,
                         const uint8_t *arr_digest, uint16_t int_status, uint64_t int_now)
{
    AstAccount_Response str_reply = response_base(str_accounts, str_request, int_status);
    save_reply(arr_digest ? &str_flow->str_reply : &str_flow->str_transient,
               &str_reply, arr_digest, int_now);
    return int_status;
}

static uint16_t handle_hello(Accounts *str_accounts, AccountFlow *str_flow,
                         const AstAccount_Request *str_request,
                         const uint8_t arr_digest[ACCOUNT_DIGEST_BYTES], uint64_t int_now)
{
    uint16_t int_status = str_flow->b_revoke_failed ? AstAccount_STORAGE_ERROR :
                          str_flow->b_revoking ? AstAccount_BUSY :
                          str_flow->int_state != FLOW_CLOSED ? AstAccount_ALREADY_ACTIVE : AstAccount_OK;
    if (int_status != AstAccount_OK)
        return reply_status(str_accounts, str_flow, str_request, NULL, int_status, int_now);
    retire_flow(str_flow);
    if (!random_nonzero(str_flow->arr_challenge, sizeof(str_flow->arr_challenge)))
        return reply_status(str_accounts, str_flow, str_request, NULL, AstAccount_STORAGE_ERROR, int_now);
    str_flow->int_state = FLOW_PREAUTH;
    str_flow->int_highwater = str_request->int_seq;
    str_flow->int_expiry = int_now + ACCOUNT_FLOW_NS;
    memcpy(str_flow->arr_nonce, str_request->arr_client_nonce, sizeof(str_flow->arr_nonce));
    AstAccount_Response str_reply = response_base(str_accounts, str_request, AstAccount_OK);
    memcpy(str_reply.arr_server_challenge, str_flow->arr_challenge, sizeof(str_reply.arr_server_challenge));
    save_reply(&str_flow->str_reply, &str_reply, arr_digest, int_now);
    return AstAccount_OK;
}

static bool allow_kdf(Accounts *str_accounts, AccountFlow *str_flow, uint64_t int_now)
{
    if (str_flow->int_last_kdf && int_now - str_flow->int_last_kdf < ACCOUNT_KDF_NS) return false;
    replenish(&str_accounts->str_kdf_budget, int_now, 2, 6);
    if (str_accounts->str_kdf_budget.int_tokens < ACCOUNT_SECOND) return false;
    str_accounts->str_kdf_budget.int_tokens -= ACCOUNT_SECOND;
    str_flow->int_last_kdf = int_now;
    return true;
}

/* A cache hit can only retransmit the reply for the same canonical request. */
static bool is_retry(AccountFlow *str_flow, const AstAccount_Request *str_request,
                     const uint8_t arr_digest[ACCOUNT_DIGEST_BYTES], uint64_t int_now)
{
    if (str_flow->str_reply.int_expiry &&
        same_bytes(str_flow->str_reply.arr_digest, arr_digest, ACCOUNT_DIGEST_BYTES)) {
        if (authenticated(str_flow, str_request)) refresh_lease(str_flow, int_now);
        str_flow->str_reply.b_send = true;
        return true;
    }
    return (str_flow->b_pending && same_bytes(str_flow->arr_pending_digest, arr_digest, ACCOUNT_DIGEST_BYTES)) ||
           (str_flow->b_revoking && same_bytes(str_flow->str_revoke.arr_digest, arr_digest, ACCOUNT_DIGEST_BYTES));
}

static uint16_t authorize_operation(const AccountFlow *str_flow, const AstAccount_Request *str_request)
{
    uint16_t int_op = str_request->int_operation;
    bool b_auth = authenticated(str_flow, str_request);
    if (int_op == AstAccount_CLOSE)
        return b_auth || (str_flow->int_state == FLOW_PREAUTH && account_zero(str_request->arr_account_session, 16)) ?
               AstAccount_OK : AstAccount_NOT_AUTHENTICATED;
    if (str_flow->b_revoke_failed) return AstAccount_STORAGE_ERROR;
    if (str_flow->b_revoking) return AstAccount_BUSY;
    if (is_login(int_op)) return str_flow->int_state == FLOW_PREAUTH ? AstAccount_OK : AstAccount_ALREADY_ACTIVE;
    if (int_op == AstAccount_FORGET && str_flow->int_state == FLOW_PREAUTH &&
        !account_zero(str_request->arr_resume_token, 32)) return AstAccount_OK;
    return b_auth ? AstAccount_OK : AstAccount_NOT_AUTHENTICATED;
}

static uint16_t handle_operation(Accounts *str_accounts, AccountFlow *str_flow,
                             const AstAccount_Request *str_request,
                             const uint8_t arr_digest[ACCOUNT_DIGEST_BYTES], uint64_t int_now,
                             bool *b_release_game)
{
    uint16_t int_op = str_request->int_operation;
    uint16_t int_status = authorize_operation(str_flow, str_request);
    if (int_status == AstAccount_OK && str_flow->b_pending && int_op != AstAccount_CLOSE)
        int_status = AstAccount_BUSY;
    if (int_status != AstAccount_OK)
        return reply_status(str_accounts, str_flow, str_request, NULL, int_status, int_now);
    if ((int_op == AstAccount_LOGIN || int_op == AstAccount_REGISTER) && !allow_kdf(str_accounts, str_flow, int_now)) {
        str_flow->int_highwater = str_request->int_seq;
        return reply_status(str_accounts, str_flow, str_request, arr_digest, AstAccount_RATE_LIMITED, int_now);
    }
    refresh_lease(str_flow, int_now);
    if (int_op == AstAccount_CLOSE) {
        *b_release_game = str_flow->b_game;
        retire_flow(str_flow);
        reply_status(str_accounts, str_flow, str_request, arr_digest, AstAccount_OK, int_now);
    } else if (int_op == AstAccount_LOGOUT || int_op == AstAccount_FORGET) {
        bool b_close = int_op == AstAccount_LOGOUT || str_flow->int_state == FLOW_PREAUTH;
        *b_release_game = b_close && str_flow->b_game;
        revoke_locked(str_accounts, str_flow, str_request, arr_digest, b_close);
    } else if (int_op == AstAccount_KEEPALIVE) {
        str_flow->int_highwater = str_request->int_seq;
        reply_status(str_accounts, str_flow, str_request, arr_digest, AstAccount_OK, int_now);
    } else {
        queue_job_locked(str_accounts, str_flow, str_request, arr_digest, int_now);
    }
    return AstAccount_OK;
}

bool accounts_receive(Accounts *str_accounts, unsigned int int_identity,
                      const AstAccount_Request *str_request, bool *b_release_game)
{
    if (!str_accounts || !identity_valid(int_identity) || !str_request || !b_release_game) return false;
    *b_release_game = false;
    uint64_t int_now = monotonic_ns();
    AccountFlow *str_flow = &str_accounts->arr_flows[int_identity - 1];
    mutex_lock(&str_accounts->str_mutex);
    replenish(&str_flow->str_input_budget, int_now, 20, 12);
    bool b_budget = str_flow->str_input_budget.int_tokens >= ACCOUNT_SECOND;
    if (b_budget) str_flow->str_input_budget.int_tokens -= ACCOUNT_SECOND;
    bool b_valid = b_budget && account_request_valid(str_request);
    bool b_audit = !b_valid && int_now - str_flow->int_last_audit >= ACCOUNT_SECOND;
    if (b_audit) str_flow->int_last_audit = int_now;
    mutex_unlock(&str_accounts->str_mutex);
    if (b_audit) audit_event(str_accounts->ptr_audit, int_identity, "ACCOUNT_DENIED", b_budget ? "invalid" : "rate");
    if (!b_valid) return true;
    uint8_t arr_digest[ACCOUNT_DIGEST_BYTES];
    if (!request_digest(str_accounts, str_request, arr_digest)) {
        OPENSSL_cleanse(arr_digest, sizeof(arr_digest));
        return false;
    }
    mutex_lock(&str_accounts->str_mutex);
    expire_flow(str_flow, int_now);
    uint16_t int_status = AstAccount_OK;
    if (!is_retry(str_flow, str_request, arr_digest, int_now)) {
        if (str_request->int_operation == AstAccount_HELLO)
            int_status = handle_hello(str_accounts, str_flow, str_request, arr_digest, int_now);
        else if (!flow_matches(str_accounts, str_flow, str_request) || str_request->int_seq <= str_flow->int_highwater)
            int_status = reply_status(str_accounts, str_flow, str_request, NULL, AstAccount_NOT_AUTHENTICATED, int_now);
        else int_status = handle_operation(str_accounts, str_flow, str_request, arr_digest, int_now, b_release_game);
    }
    b_audit = int_status != AstAccount_OK && int_now - str_flow->int_last_audit >= ACCOUNT_SECOND;
    if (b_audit) str_flow->int_last_audit = int_now;
    mutex_unlock(&str_accounts->str_mutex);
    OPENSSL_cleanse(arr_digest, sizeof(arr_digest));
    if (b_audit) audit_event(str_accounts->ptr_audit, int_identity, "ACCOUNT_DENIED",
                             int_status == AstAccount_RATE_LIMITED ? "rate" : "flow");
    return true;
}

/* Round-robin polling bounds the DDS work even when every identity is requesting. */
bool accounts_next_response(Accounts *str_accounts, unsigned int *int_identity,
                            AstAccount_Response *str_response)
{
    if (!str_accounts || !int_identity || !str_response) return false;
    uint64_t int_now = monotonic_ns();
    bool b_found = false;
    mutex_lock(&str_accounts->str_mutex);
    replenish(&str_accounts->str_reply_budget, int_now, 10, 6);
    for (unsigned int int_n = 0; int_n < AST_IDENTITIES; ++int_n) {
        unsigned int int_index = (str_accounts->int_next_reply + int_n) % AST_IDENTITIES;
        AccountFlow *str_flow = &str_accounts->arr_flows[int_index];
        expire_flow(str_flow, int_now);
        if (b_found || str_accounts->str_reply_budget.int_tokens < ACCOUNT_SECOND) continue;
        AccountReply *str_reply = str_flow->str_reply.b_send ? &str_flow->str_reply : &str_flow->str_transient;
        replenish(&str_flow->str_reply_budget, int_now, 5, 5);
        if (!str_reply->b_send || str_flow->str_reply_budget.int_tokens < ACCOUNT_SECOND) continue;
        if (str_flow->int_response_seq == UINT64_MAX) {
            atomic_store(&str_accounts->b_failed, true);
            break;
        }
        *str_response = str_reply->str_message;
        str_response->int_seq = ++str_flow->int_response_seq;
        *int_identity = int_index + 1;
        str_reply->b_send = false;
        str_flow->str_reply_budget.int_tokens -= ACCOUNT_SECOND;
        str_accounts->str_reply_budget.int_tokens -= ACCOUNT_SECOND;
        b_found = true;
    }
    if (b_found) str_accounts->int_next_reply = *int_identity % AST_IDENTITIES;
    mutex_unlock(&str_accounts->str_mutex);
    return b_found;
}

bool accounts_authorize_join(Accounts *str_accounts, unsigned int int_identity,
                             const uint8_t arr_nonce[16], uint64_t *int_account_id)
{
    if (!str_accounts || !identity_valid(int_identity) || !arr_nonce || !int_account_id) return false;
    mutex_lock(&str_accounts->str_mutex);
    AccountFlow *str_flow = &str_accounts->arr_flows[int_identity - 1];
    expire_flow(str_flow, monotonic_ns());
    bool b_ok = str_flow->int_state == FLOW_AUTHENTICATED &&
                same_bytes(str_flow->arr_nonce, arr_nonce, sizeof(str_flow->arr_nonce));
    *int_account_id = b_ok ? str_flow->int_account_id : 0;
    mutex_unlock(&str_accounts->str_mutex);
    return b_ok;
}

/* Commit admission against the same flow before the caller allocates the game slot. */
bool accounts_game_joined(Accounts *str_accounts, unsigned int int_identity,
                          const uint8_t arr_nonce[16], uint64_t int_account_id)
{
    if (!str_accounts || !identity_valid(int_identity) || !arr_nonce || !int_account_id) return false;
    mutex_lock(&str_accounts->str_mutex);
    AccountFlow *str_flow = &str_accounts->arr_flows[int_identity - 1];
    expire_flow(str_flow, monotonic_ns());
    bool b_ok = str_flow->int_state == FLOW_AUTHENTICATED && str_flow->int_account_id == int_account_id &&
                same_bytes(str_flow->arr_nonce, arr_nonce, sizeof(str_flow->arr_nonce));
    if (b_ok) {
        str_flow->b_game = true;
        /* The player has used the grant. Its cached bearer reply is now unnecessary. */
        if (is_login(str_flow->str_reply.str_message.int_operation))
            OPENSSL_cleanse(&str_flow->str_reply, sizeof(str_flow->str_reply));
    }
    mutex_unlock(&str_accounts->str_mutex);
    return b_ok;
}

void accounts_game_left(Accounts *str_accounts, unsigned int int_identity, const uint8_t arr_nonce[16])
{
    if (!str_accounts || !identity_valid(int_identity) || !arr_nonce) return;
    mutex_lock(&str_accounts->str_mutex);
    AccountFlow *str_flow = &str_accounts->arr_flows[int_identity - 1];
    if (str_flow->b_game && same_bytes(str_flow->arr_nonce, arr_nonce, sizeof(str_flow->arr_nonce)))
        retire_flow(str_flow);
    mutex_unlock(&str_accounts->str_mutex);
}

/* Store an immutable max candidate. A concurrent higher version survives a commit. */
bool accounts_submit_score(Accounts *str_accounts, unsigned int int_identity, const AccountScore *str_score)
{
    if (!str_accounts || !identity_valid(int_identity) || !str_score ||
        !str_score->int_account_id || str_score->int_account_id > INT64_MAX) return false;
    mutex_lock(&str_accounts->str_mutex);
    PendingScore *str_pending = &str_accounts->arr_scores[int_identity - 1];
    bool b_ok = !str_accounts->b_stop && !str_accounts->b_score_failed &&
        (!str_pending->int_version || str_pending->str_score.int_account_id == str_score->int_account_id);
    if (b_ok && (!str_pending->int_version || str_score->int_score > str_pending->str_score.int_score)) {
        str_pending->str_score = *str_score;
        ++str_pending->int_version;
        str_pending->b_pending = true;
        checked_sync(pthread_cond_signal(&str_accounts->str_cond));
    }
    mutex_unlock(&str_accounts->str_mutex);
    return b_ok;
}

typedef enum { WORK_NONE, WORK_REQUEST, WORK_REVOKE, WORK_SCORE } WorkKind;

typedef struct {
    WorkKind int_kind;
    unsigned int int_identity;
    AccountJob str_job;
    PendingScore str_score;
} AccountWork;

/* Only the storage thread takes jobs. The in-flight digest stays for retry matching. */
static bool take_work(Accounts *str_accounts, AccountWork *str_work, WorkKind int_kind)
{
    for (unsigned int int_n = 0; int_n < AST_IDENTITIES; ++int_n) {
        unsigned int int_index = (str_accounts->int_next_job + int_n) % AST_IDENTITIES;
        AccountFlow *str_flow = &str_accounts->arr_flows[int_index];
        if (int_kind == WORK_SCORE) {
            if (str_accounts->b_score_failed || !str_accounts->arr_scores[int_index].b_pending) continue;
            str_work->str_score = str_accounts->arr_scores[int_index];
        } else {
            if (int_kind == WORK_REQUEST) expire_flow(str_flow, monotonic_ns());
            AccountJob *str_job = int_kind == WORK_REVOKE ? &str_flow->str_revoke : &str_flow->str_job;
            if (!str_job->b_queued) continue;
            str_work->str_job = *str_job;
            OPENSSL_cleanse(str_job, sizeof(*str_job));
            if (int_kind == WORK_REVOKE)
                memcpy(str_job->arr_digest, str_work->str_job.arr_digest, ACCOUNT_DIGEST_BYTES);
        }
        str_work->int_kind = int_kind;
        str_work->int_identity = int_index + 1;
        str_accounts->int_next_job = (int_index + 1) % AST_IDENTITIES;
        return true;
    }
    return false;
}

/* Finish reserved durable work after stop. Password jobs can be discarded. */
static bool wait_work(Accounts *str_accounts, AccountWork *str_work)
{
    mutex_lock(&str_accounts->str_mutex);
    while (!take_work(str_accounts, str_work, WORK_REVOKE) && !take_work(str_accounts, str_work, WORK_SCORE)) {
        if (str_accounts->b_stop) break;
        if (take_work(str_accounts, str_work, WORK_REQUEST)) break;
        checked_sync(pthread_cond_wait(&str_accounts->str_cond, &str_accounts->str_mutex));
    }
    mutex_unlock(&str_accounts->str_mutex);
    return str_work->int_kind != WORK_NONE;
}

static uint16_t store_status(StoreResult int_result)
{
    switch (int_result) {
    case STORE_OK: return AstAccount_OK;
    case STORE_INVALID_CREDENTIALS: return AstAccount_INVALID_CREDENTIALS;
    case STORE_REGISTRATION_DENIED: return AstAccount_REGISTRATION_DENIED;
    default: return AstAccount_STORAGE_ERROR;
    }
}

static bool copy_record(AstAccount_Response *str_reply, const AccountRecord *str_record)
{
    size_t int_name_length = strnlen(str_record->str_username, sizeof(str_record->str_username));
    if (!account_username_valid((const uint8_t *)str_record->str_username, int_name_length)) return false;
    str_reply->int_account_id = str_record->int_account_id;
    str_reply->int_own_best = str_record->int_best_score;
    str_reply->int_username_length = (uint16_t)int_name_length;
    memcpy(str_reply->arr_username, str_record->str_username, int_name_length);
    return true;
}

/* Reserve a new ephemeral session before any credential transaction rotates a token. */
static StoreResult login_job(AccountStore *str_store, const AccountWork *str_work,
                             AstAccount_Response *str_reply)
{
    const AstAccount_Request *str_request = &str_work->str_job.str_message;
    if (!random_nonzero(str_reply->arr_account_session, sizeof(str_reply->arr_account_session))) return STORE_ERROR;
    AccountGrant str_grant = {0};
    StoreResult int_result;
    if (str_request->int_operation == AstAccount_RESUME) {
        int_result = account_store_resume(str_store, str_work->int_identity, str_request->arr_resume_token, &str_grant);
    } else {
        char str_username[sizeof(str_request->arr_username) + 1] = {0};
        memcpy(str_username, str_request->arr_username, str_request->int_username_length);
        int_result = account_store_auth(str_store, str_request->int_operation == AstAccount_REGISTER,
                                       str_work->int_identity, str_username, str_request->arr_password,
                                       str_request->int_password_length, str_request->b_remember, &str_grant);
    }
    if (int_result == STORE_OK) {
        if (!copy_record(str_reply, &str_grant.str_account)) int_result = STORE_ERROR;
        else if (str_grant.b_token_present) {
            str_reply->b_token_present = true;
            str_reply->int_token_expiry_unix_s = str_grant.int_token_expiry_unix_s;
            memcpy(str_reply->arr_resume_token, str_grant.arr_token, sizeof(str_grant.arr_token));
        }
    }
    OPENSSL_cleanse(&str_grant, sizeof(str_grant));
    return int_result;
}

static StoreResult leaderboard_job(AccountStore *str_store, const AccountJob *str_job,
                                   AstAccount_Response *str_reply)
{
    AccountRecord str_own = {0}, arr_rows[ACCOUNT_STORE_LEADERBOARD_MAX] = {0};
    size_t int_count = 0;
    StoreResult int_result = account_store_leaderboard(str_store, str_job->int_account_id, &str_own, arr_rows, &int_count);
    if (int_result != STORE_OK) return int_result;
    if (int_count > sizeof(arr_rows) / sizeof(arr_rows[0]) || !copy_record(str_reply, &str_own)) return STORE_ERROR;
    for (size_t int_i = 0; int_i < int_count; ++int_i) {
        const AccountRecord *str_row = &arr_rows[int_i];
        size_t int_length = strnlen(str_row->str_username, sizeof(str_row->str_username));
        if (!account_username_valid((const uint8_t *)str_row->str_username, int_length)) return STORE_ERROR;
        AstAccount_LeaderEntry *str_entry = &str_reply->arr_entries[int_i];
        str_entry->int_account_id = str_row->int_account_id;
        str_entry->int_best_score = str_row->int_best_score;
        str_entry->int_updated_unix_s = str_row->int_updated_unix_s;
        str_entry->int_username_length = (uint16_t)int_length;
        memcpy(str_entry->arr_username, str_row->str_username, int_length);
    }
    str_reply->int_entry_count = (uint16_t)int_count;
    return STORE_OK;
}

static void score_job(Accounts *str_accounts, AccountStore *str_store, const AccountWork *str_work)
{
    bool b_ok = account_store_score(str_store, &str_work->str_score.str_score) == STORE_OK;
    mutex_lock(&str_accounts->str_mutex);
    PendingScore *str_score = &str_accounts->arr_scores[str_work->int_identity - 1];
    if (b_ok) {
        if (str_score->int_version == str_work->str_score.int_version) str_score->b_pending = false;
    } else {
        str_accounts->b_score_failed = true;
        atomic_store(&str_accounts->b_failed, true);
        notify_locked(str_accounts);
    }
    mutex_unlock(&str_accounts->str_mutex);
    audit_event(str_accounts->ptr_audit, str_work->int_identity,
                b_ok ? "ACCOUNT_SCORE_SAVED" : "ACCOUNT_STORAGE_ERROR", b_ok ? "committed" : "score");
}

static const char *account_operation(uint16_t int_op)
{
    switch (int_op) {
    case AstAccount_REGISTER: return "register";
    case AstAccount_LOGIN: return "login";
    case AstAccount_RESUME: return "resume";
    case AstAccount_LOGOUT: return "logout";
    case AstAccount_FORGET: return "forget";
    default: return "leaderboard";
    }
}

/* Complete under the flow lock only if no timeout, CLOSE or Leave changed its generation. */
static bool complete_job(Accounts *str_accounts, const AccountWork *str_work,
                         const AstAccount_Response *str_reply)
{
    uint64_t int_now = monotonic_ns();
    bool b_revoke = str_work->int_kind == WORK_REVOKE;
    bool b_ok = str_reply->int_status == AstAccount_OK;
    mutex_lock(&str_accounts->str_mutex);
    AccountFlow *str_flow = &str_accounts->arr_flows[str_work->int_identity - 1];
    expire_flow(str_flow, int_now);
    if (b_revoke) {
        str_flow->b_revoking = false;
        str_flow->b_revoke_failed = str_reply->int_status == AstAccount_STORAGE_ERROR;
        OPENSSL_cleanse(&str_flow->str_revoke, sizeof(str_flow->str_revoke));
    }
    bool b_current = !str_accounts->b_stop && str_flow->int_generation == str_work->str_job.int_generation &&
                     (b_revoke || str_flow->b_pending);
    if (b_current) {
        str_flow->b_pending = false;
        OPENSSL_cleanse(str_flow->arr_pending_digest, sizeof(str_flow->arr_pending_digest));
        if (is_login(str_reply->int_operation)) {
            if (b_ok) {
                str_flow->int_state = FLOW_AUTHENTICATED;
                str_flow->int_account_id = str_reply->int_account_id;
                str_flow->int_expiry = int_now + ACCOUNT_LEASE_NS;
                memcpy(str_flow->arr_session, str_reply->arr_account_session, sizeof(str_flow->arr_session));
            } else retire_flow(str_flow);
        }
        save_reply(&str_flow->str_reply, str_reply, str_work->str_job.arr_digest, int_now);
        notify_locked(str_accounts);
    }
    mutex_unlock(&str_accounts->str_mutex);
    return b_current;
}

static void run_job(Accounts *str_accounts, AccountStore *str_store, const AccountWork *str_work)
{
    const AstAccount_Request *str_request = &str_work->str_job.str_message;
    AstAccount_Response str_reply = response_base(str_accounts, str_request, AstAccount_OK);
    StoreResult int_result;
    if (str_work->int_kind == WORK_REVOKE) {
        uint64_t int_id = str_work->str_job.int_account_id;
        int_result = account_store_revoke(str_store, str_work->int_identity, int_id,
                                          int_id ? NULL : str_request->arr_resume_token);
    } else if (str_request->int_operation == AstAccount_LEADERBOARD) {
        int_result = leaderboard_job(str_store, &str_work->str_job, &str_reply);
    } else {
        int_result = login_job(str_store, str_work, &str_reply);
    }
    if (int_result == STORE_OK) {
        str_reply.b_persisted = true;
        str_reply.int_seq = 1; /* Structural validation. The DDS owner assigns the wire sequence. */
        if (!account_response_valid(&str_reply)) int_result = STORE_ERROR;
    }
    if (int_result != STORE_OK) {
        OPENSSL_cleanse(&str_reply, sizeof(str_reply));
        str_reply = response_base(str_accounts, str_request, store_status(int_result));
    }
    bool b_current = complete_job(str_accounts, str_work, &str_reply);
    if ((b_current && str_request->int_operation != AstAccount_LEADERBOARD) || str_work->int_kind == WORK_REVOKE)
        audit_event(str_accounts->ptr_audit, str_work->int_identity,
                    int_result == STORE_OK ? "ACCOUNT_OK" : "ACCOUNT_DENIED",
                    int_result == STORE_OK ? account_operation(str_request->int_operation) : "credentials-or-storage");
    OPENSSL_cleanse(&str_reply, sizeof(str_reply));
}

/* This is the only thread allowed to open, query, update or close SQLite. */
static void *storage_worker(void *ptr_context)
{
    Accounts *str_accounts = ptr_context;
    AccountStore str_store = {0};
    bool b_open = account_store_open(&str_store, str_accounts->str_data_dir);
    mutex_lock(&str_accounts->str_mutex);
    str_accounts->b_open = b_open;
    str_accounts->b_started = true;
    checked_sync(pthread_cond_signal(&str_accounts->str_cond));
    mutex_unlock(&str_accounts->str_mutex);
    if (!b_open) return NULL;
    AccountWork str_work = {0};
    while (wait_work(str_accounts, &str_work)) {
        if (str_work.int_kind == WORK_SCORE) score_job(str_accounts, &str_store, &str_work);
        else run_job(str_accounts, &str_store, &str_work);
        OPENSSL_cleanse(&str_work, sizeof(str_work));
    }
    if (!account_store_close(&str_store)) atomic_store(&str_accounts->b_failed, true);
    return NULL;
}

Accounts *accounts_create(const char *str_data_dir, Audit *str_audit)
{
    if (!str_data_dir || !str_audit) return NULL;
    Accounts *str_accounts = calloc(1, sizeof(*str_accounts));
    if (!str_accounts) return NULL;
    atomic_init(&str_accounts->b_failed, false);
    str_accounts->str_data_dir = str_data_dir;
    str_accounts->ptr_audit = str_audit;
    if (!random_nonzero(str_accounts->arr_epoch, sizeof(str_accounts->arr_epoch)) ||
        !random_nonzero(str_accounts->arr_hmac_key, sizeof(str_accounts->arr_hmac_key))) goto fail;
    if (pthread_mutex_init(&str_accounts->str_mutex, NULL) != 0) goto fail;
    if (pthread_cond_init(&str_accounts->str_cond, NULL) != 0) goto fail_mutex;
    if (pthread_create(&str_accounts->str_thread, NULL, storage_worker, str_accounts) != 0) goto fail_cond;
    mutex_lock(&str_accounts->str_mutex);
    while (!str_accounts->b_started)
        checked_sync(pthread_cond_wait(&str_accounts->str_cond, &str_accounts->str_mutex));
    bool b_open = str_accounts->b_open;
    mutex_unlock(&str_accounts->str_mutex);
    if (b_open) return str_accounts;
    (void)accounts_destroy(str_accounts);
    return NULL;
fail_cond:
    checked_sync(pthread_cond_destroy(&str_accounts->str_cond));
fail_mutex:
    checked_sync(pthread_mutex_destroy(&str_accounts->str_mutex));
fail:
    OPENSSL_cleanse(str_accounts, sizeof(*str_accounts));
    free(str_accounts);
    return NULL;
}

void accounts_set_events(Accounts *str_accounts, EventQueue *str_events)
{
    if (!str_accounts) return;
    mutex_lock(&str_accounts->str_mutex);
    str_accounts->ptr_events = str_events;
    mutex_unlock(&str_accounts->str_mutex);
}

bool accounts_failed(Accounts *str_accounts)
{
    return !str_accounts || atomic_load(&str_accounts->b_failed);
}

bool accounts_destroy(Accounts *str_accounts)
{
    if (!str_accounts) return true;
    mutex_lock(&str_accounts->str_mutex);
    str_accounts->ptr_events = NULL;
    str_accounts->b_stop = true;
    for (unsigned int int_i = 0; int_i < AST_IDENTITIES; ++int_i)
        retire_flow(&str_accounts->arr_flows[int_i]);
    checked_sync(pthread_cond_signal(&str_accounts->str_cond));
    mutex_unlock(&str_accounts->str_mutex);
    checked_sync(pthread_join(str_accounts->str_thread, NULL));
    bool b_ok = str_accounts->b_open && !accounts_failed(str_accounts);
    for (unsigned int int_i = 0; int_i < AST_IDENTITIES; ++int_i)
        if (str_accounts->arr_flows[int_i].b_revoke_failed || str_accounts->arr_scores[int_i].b_pending) b_ok = false;
    checked_sync(pthread_cond_destroy(&str_accounts->str_cond));
    checked_sync(pthread_mutex_destroy(&str_accounts->str_mutex));
    OPENSSL_cleanse(str_accounts, sizeof(*str_accounts));
    free(str_accounts);
    return b_ok;
}
