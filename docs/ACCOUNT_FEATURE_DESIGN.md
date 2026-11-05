# Jordain account features — approved design

Prepared 5 November 2026 from Stanley's TEAMMATE_HANDOVER_2026-11-05.md and the source at `480227c` plus preserved local work. The Telegram attachment and repository handover have identical text after newline normalization. See [approval](ACCOUNT_APPROVAL.md), [current progress](PROGRESS.md) and [runtime results](TEST_REPORT.md).

## Assigned result and proposed decisions

| Feature | Proposed behavior |
|---|---|
| Account login | GLFW login/register screen; certificate authentication followed by server-checked username/password before a game slot is allocated |
| Saved high scores | Server-earned best score survives a server restart; authenticated top-10 leaderboard and own best score |
| Remember me | Explicit opt-in, rotating seven-day token; password never saved; logout/forget revoke it |
| Registration | One account per provisioned certificate identity: at most six accounts, each permanently bound to its own logical identity |
| Parallel logins | One active account/client nonce per certificate identity, one active login per account; reject newer instances without taking over a current player |
| Scores on departure | Save the authoritative participation score on explicit Leave, lease expiry, game-over and before restart/reset; graceful server shutdown drains pending saves |
| Database | Propose SQLite, owned by one added account/storage worker; requires explicit dependency approval and human installation |
| Ordinary window close | Leaves gameplay and clears ephemeral account authority; preserves an opted-in remembered token. Explicit Logout/Forget clears it |

These defaults form one reviewable proposal. Changing an account policy is possible before implementation. Six logical certificate identities and five gameplay slots remain distinct limits.

## J0 evidence before implementation

At J0 the code used certificate-only admission and kept scores in memory. The combined pulled checkout passed build, unit, hardening, native P1 and timeout-fallback checks before implementation.

Ubuntu has OpenSSL 3.0.13 with `EVP_PBE_scrypt`. A separate feasibility probe compiled warning-free and exited 0 using N=131072, r=8, p=1, maxmem=167772160 bytes, a 16-byte random salt and 32-byte output. Three derivations took 428.315367, 408.776511 and 336.227400 ms; process peak RSS was 136600 KiB. Same-input verification and changed-input rejection passed. No password, salt, hash or token value was printed. This is a local feasibility sample, not a gameplay/flood benchmark.

SQLite C development support is missing: no `sqlite3.h`, and `pkg-config --modversion sqlite3` exits 1. If the team approves this dependency, the human runs inside Ubuntu:

```bash
sudo apt update
sudo apt install -y libsqlite3-dev
```

The sqlite3 command-line program is optional. No package installation is performed by this design work.

## Authority, protocol and compatibility

The concrete draft is [proposals/accounts_v1.idl](proposals/accounts_v1.idl); its patch proposes a new `idl/accounts.idl`. It is outside the game build. Existing `Ast` gameplay types, exact topics and `AST_VERSION=2` keep their current layout. Account messages use independent version 1 and fixed arrays/counts, with no unbounded strings or client-supplied score/account authority.

Add exactly two topic families for each identity i=1..6:

- `DDSAsteroids_AccountRequest_i`: only client i publishes; only server reads.
- `DDSAsteroids_AccountResponse_i`: only server publishes; only client i reads.

Use explicit reliable KEEP_LAST(1), bounded resource limits and the existing finite write deadline. The current endpoint helper special-cases only `Response`; it must deliberately handle `AccountResponse` as well. Clients keep one request outstanding and retry its identical canonical fields after loss. Per identity, keep a sequence high-water mark for the live flow and one completed reply for at most 30 seconds. Match retries against a process-keyed HMAC of the whole canonical request, including credential/remember fields, without retaining the password. Reused correlation with changed arguments is invalid. Cache expiry does not reset the live flow's high-water mark. A cached token reply is transient secret memory, never persisted/logged, cleansed on expiry/replacement or authority-generation change. A bounded nonsecret terminal acknowledgement may remain retryable after closing.

Operations: HELLO, REGISTER, LOGIN, RESUME, LOGOUT, LEADERBOARD, FORGET, KEEPALIVE and CLOSE. HELLO returns the per-server-process epoch and a fresh random 128-bit per-flow challenge valid for 30 seconds. Subsequent requests must echo both. Only a currently issued, unconsumed preauthentication challenge can start REGISTER/LOGIN/RESUME; success moves that flow to authenticated state and cannot be replayed as a new login. Closing/expiry retires the challenge; a new HELLO creates a different one, so old requests cannot reopen authority even in the same server epoch. HELLO cannot displace an active flow, refresh an active lease or extend a queued-job deadline. REGISTER/LOGIN/RESUME begin without an account session; success establishes a random 128-bit account session. Authenticated operations echo it. FORGET may alternatively revoke a supplied token by possession plus its correct certified identity.

Every request has nonzero sequence/client/request nonces. Sequence numbers increase within its flow, never wrap/reset; exact retries alone reuse a sequence. Auth jobs have a ten-second deadline; late results cannot grant authority. An authenticated account without a gameplay slot has a ten-second lease renewed by a cheap KEEPALIVE about once per second. Valid authenticated retries also demonstrate liveness; HELLO does not. During gameplay the existing three-second input lease remains authoritative. CLOSE works even before Join: it drops ephemeral account authority while preserving a remembered token. Crashes expire by lease rather than an assumed instantaneous disconnect. KEEPALIVE/CLOSE require no KDF and cannot be blocked behind password work; any needed persistence cleanup is queued separately.

The challenge's 30-second timeout applies before authentication. After success, keep it as a binding for that authenticated flow; the account/game lease controls its lifetime. A subsequent login flow obtains a new challenge. This design does not add automatic recovery of an existing game after server restart; restart the client/new login flow to resume the remembered account.

| Operation | Required fields beyond common correlation; all other unused fields zero |
|---|---|
| HELLO | Epoch/challenge/session/token/credentials zero; remember false |
| REGISTER / LOGIN | Current epoch and issued challenge; valid username/password; account session/token zero; remember optional |
| RESUME | Current epoch/challenge and nonzero token; session/credentials zero; remember true because rotation is mandatory |
| LOGOUT / LEADERBOARD / KEEPALIVE | Current epoch/challenge/account session; token/credentials zero; remember false |
| CLOSE | Current epoch/challenge and client nonce; current account session if authenticated, zero session if cancelling a live preauthentication/KDF flow; token/credentials zero; remember false |
| FORGET | Current epoch/challenge; exactly one of a valid account session or token; credentials zero; remember false |

HELLO success returns only epoch/challenge/correlation. Authentication success returns valid account/name/best/session and a token only when requested/rotated. LEADERBOARD returns committed own metadata and at most ten entries, never a token or newly issued session. Error/Logout/Forget/Close/Keepalive responses have no account/session/token/name/score/leaderboard data. Zero unused array bytes/rows. Account IDs are positive signed-64-bit values; scores are at most UINT32_MAX, and timestamps fit signed 64-bit storage. `b_persisted` means a completed database read/commit for successful authentication, leaderboard or durable revocation; it is false for errors, HELLO, KEEPALIVE and in-memory CLOSE. It never asserts that the client profile was saved.

Generate a random client nonce for each authentication flow. Reuse it in existing `Ast_Join.arr_nonce`; retain it for retries, then refresh after logout/re-authentication so retired Join nonces are not reused. Responses echo operation, client/request nonces and request sequence. The DDS worker validates all fields and operation-specific unused-zero requirements before queueing work. Unknown statuses/operations, wrong epochs and stale/mismatched correlations fail closed. Wrong username/password/binding share a generic invalid-credentials error.

Server derives identity from the authenticated reader channel, never from a client field. It accepts unchanged Join only when that channel and Join nonce match current authenticated account state. `allocate_slot()` receives a server-owned account mapping; no account request allocates a gameplay slot. Six authenticated accounts can exist while only five are playing. Native certificate/role enforcement remains mandatory; there is no unauthenticated guest fallback.

Server tags 0..17 remain Join/Input/Leave. Six account-request readers use 18..23; internal worker completion uses bit 24. Thus 24 DDS readers and 25 event bits fit the 32-bit mask. Update every array, dispatch branch, queue bound and compile-time assertion together. Client gains a third DDS reader plus a bounded UI-command notification path.

Governance already covers topics with encryption/access checks, but permissions must enumerate the new exact topics. Old clients lack the account flow and old bundles lack its permissions. This is an application-compatibility change despite keeping the old gameplay structs. Regenerate/re-sign permissions and adapt all bot/GUI/security harnesses together after approval.

Account messages must remain below 2 KiB serialized payload; measure both CDR representations in acceptance. Reuse typed outgoing padding protection for the new request/response and their fixed leaderboard rows.

## Passwords, limits and worker model

Usernames: 3–24 lowercase ASCII bytes `[a-z0-9_]`, globally unique. Uppercase is rejected with a clear format hint; no hidden case conversion. Passwords: 12–128 printable ASCII bytes, including space; no trimming/case folding. This intentionally limited first-version input policy needs team acceptance; it is not a general Unicode account service. GLFW character callbacks add typed entry, backspace, tab/focus, masked password, submit, loading/error states and logout. No new rendering framework.

Use OpenSSL scrypt with the probed N/r/p, a fresh 16-byte salt, 32-byte verifier, explicit 160 MiB KDF memory cap, checked return values and constant-time comparison. Persist salt/verifier/algorithm parameters only. Use a dummy verifier calculation for unknown names; retain generic errors. Clear owned password buffers after processing. Internal DDS history is bounded; do not promise every library copy is wiped.

One added account/storage worker owns the SQLite connection and performs all KDF/database work. Existing simulation and DDS callbacks do not perform it. Bounds: six account requests, six results, one outstanding request per identity and one active KDF. Throttle before expensive work: one new password KDF per identity per two seconds, global token bucket two/second with an initial burst of six. A full queue returns BUSY; malformed requests do not consume KDF work. Benchmark legitimate gameplay/flood behavior before accepting these initial rates.

Reserve bounded control/cancellation state per identity. Every job/result carries trusted identity, client nonce, request nonce/sequence and authentication generation. Accepted LOGOUT/CLOSE or detected lease expiry invalidates authority immediately; stale completions never install a session or emit a secret reply. Worker handles queued revocation consistently; no account/game mutex is held through KDF, SQLite calls or DDS writes.

CLOSE is the explicit exception to the one-outstanding ordinary RPC rule: its reserved control path can cancel a pending REGISTER/LOGIN/RESUME before an account session exists. Verify the live identity/client nonce/epoch/challenge, advance sequence/generation, retire the flow and discard its eventual completion. An account row or rotation committed just before cancellation is not silently undone; interrupted token delivery can require password login. In particular, closing a pending flow must not promise that an earlier resume token remains usable after a committed rotation.

Revocation ordering is explicit: cancel increments the authority generation and cleanses cached secret replies immediately. The single storage worker finishes any already-running job, discards its stale result, and processes the reserved revoke before any newer authentication for that identity. Queued older jobs are skipped. A revocation-success response is sent only after its transaction commits, so no older job can create a usable token after that acknowledgement. If storage fails, report failure and keep the identity closed to new authentication until handled; do not claim durable revocation. Profile deletion alone cannot revoke a copied token.

Cheap HELLO, denial, BUSY and KEEPALIVE responses also have bounded work: initial proposal is at most five replies/second per identity and ten/second globally (burst six), at most two response writes per worker turn. Excess denials may be silently dropped; conforming clients retry their same request. Do not perform a KDF or unbounded logging for rejected traffic. Validate responsiveness with the real flood tests before accepting these budgets.

The score path uses a separate fixed pending-max mailbox of six keyed account entries, indexed through the validated provisioned-identity mapping rather than raw SQL account IDs. Captures coalesce to the highest pending candidate per account without losing the best score or blocking simulation behind password work. Each entry has a generation/version: after committing snapshot v1, clear only if the entry still equals v1; preserve a concurrent higher v2 for the next commit. Keep pending data until successful commit; storage failure is explicit, and the UI says saved only after commit. Worker services pending score/control work between KDF jobs.

## Storage and authoritative score capture

Draft schema: [proposals/accounts_schema_v1.sql](proposals/accounts_schema_v1.sql). Store account id/name/bound identity, verifier parameters, best score/provenance and hashed token metadata. Use prepared statements, foreign keys, checked transactions, unique identity binding and a six-account capacity constraint. One private server database, no shared connection with other application threads.

Runtime root is the server's private Linux home at `$HOME/.local/state/dds-asteroids/server-<CA fingerprint>/`. Require an owned nonsymlink 0700 directory, 0600 database/journal/lock files, umask077, SQLite NOFOLLOW and one server lock. Fail on corruption, unknown schema, unsafe ownership/path or failed migration; preserve the existing file. Only initialize a genuinely new database. Use rollback journal and FULL synchronization; migrations are transactions, never silent resets.

Capture immutable score candidates under the existing game mutex, then service them outside it. Candidates contain account id, server-boot-plus-round key, gameplay session and server-computed score. Retain simulation-owner account/session metadata separately from current Slot so rapid reuse cannot credit an old ship to a new player.

Actual destructive hooks that must be covered:

1. `net_server.c:leave_message()` before Slot is cleared.
2. `main.c:server_expire()` before timed-out Slot is cleared.
3. `main.c:game_step()` before `sim_connect()` replaces a previous ship.
4. Game-over transition, and **before** `sim_tick()` performs restart: its restart routine zeroes scores before the caller observes the restart counter.
5. Graceful server shutdown before account worker drains and exits.

Transactional max update changes the stored best only when a candidate is larger. Equal/duplicate/lower candidates are no-ops, including timestamps; record the winning account/round/session provenance. This makes retries idempotent without an unbounded event-history table. Coalescing pending candidates preserves this max behavior. A leaderboard includes only committed data and deterministically sorts by best descending, then username. Return at most ten rows and own best, never client-provided final scores.

Client crash is handled by the existing lease and score capture. Server restart preserves committed bests; forced server death can lose an uncommitted current round and is not represented as guaranteed durability.

## Remember, rotation and forgetting

Remember me is opt-in. Issue 32 random bytes; persist only SHA-256(token) server-side, bound to account, provisioned logical identity, password generation, server scope and seven-day UTC expiry. A token is high-entropy random data; this hash is not a replacement for password scrypt.

Resume still requires the correct native certificate channel, current HELLO epoch and fresh correlation. Atomically replace the stored token hash on successful resume; old tokens fail. Exact same-request retries may receive the bounded cached result. If a server crash/lost reply makes that result unrecoverable, return to password login. Never retain multiple usable old tokens to hide a rotation failure.

Store username/raw token only in an owned 0600 profile under a nonsymlink owned 0700 Linux-home directory. Separate by trusted server scope and certificate identity; lock the profile against concurrent writers. Use atomic replace/fsync and do profile writes before Join, while no gameplay heartbeat is due. A failed profile write reports that remembering failed and revokes/abandons the new token safely. No passwords or ephemeral account sessions in files.

Password login invalidates older remembered tokens; issue a new one only when opted in. Explicit logout/online forget revoke and clear local state. Ordinary closure keeps the remembered token but drops account/game authority. Offline forget clears the profile and clearly reports that immediate server revocation is unconfirmed. Any later password-change/reset feature must increment password generation and revoke tokens; it is not silently added in this first UI.

This binding is to the CA-provisioned logical identity/channel, not an exact certificate fingerprint. Renewal with the same provisioned identity remains usable. Exact fingerprint binding would require a trusted server provisioning map and a separate renewal policy; never trust a client-reported fingerprint.

One certificate identity is one trusted principal in this version. Every holder of that identity's private key can subscribe to its account-response topic; nonce routing does not hide bearer replies from another holder of the same certificate. Therefore accounts for separate players use distinct provisioned identities, and tests of account isolation use different identities. Sharing a credential is sharing the principal, not a supported boundary between users. Existing Snapshot visibility remains certificate-level, including before account login; the new account gate controls participation and account data rather than adding per-account world encryption.

## Implementation order and acceptance

After Stanley approves the proposal and SQLite is installed:

1. Add account/storage worker, schema/verifier tests, request/response types, narrow permissions and account-before-Join enforcement.
2. Add real login/register UI and private headless test input; no secret command-line flags or secret-bearing logs.
3. Add score capture/mailbox/transactional max and leaderboard UI; test real earned scores and storage failure/restart.
4. Add profile/token rotation/logout/forget, then existing lifecycle/security/strict two-window regression checks.

Required evidence: correct certificate+account; wrong/unknown credentials use no slot; binding/duplicate/rate/queue limits; boundary and malformed fields; real authoritative scores, duplicates/lower scores/account isolation and restart persistence; corruption/unwritable paths preserve files; remember after client restart, expiry/wrong binding/old-token replay/revocation/forget; two distinct authenticated windows still move/fire/leave normally. Use fresh output directories, private generated fixtures and nonzero failing exit codes. No stored password/token values in screenshots, logs, Git or source ZIP.

Change together: `Makefile`, new `idl/accounts.idl`, common account validators/bounds/wire helpers, server accounts/storage/score ownership, client login/profile/UI/network state, exact permissions generator, launchers and affected tests. Update README, threat model, protocol/evidence/AI documentation and ignore/archive rules for databases, journals, backups and profiles. Existing local padding/diagnostic work is preserved. Excluded native-dependency investigations are not reopened by J0.

Stanley's manual acceptance is separate from automated verification.

## References and review gate

- Team-supplied handover J0: “Obtain Stanley's approval BEFORE applying a breaking IDL/topic/policy change.”
- [OpenSSL 3.0 scrypt parameters and memory limit](https://docs.openssl.org/3.0/man7/EVP_KDF-SCRYPT/).
- [OWASP password-storage guidance](https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html) supports the selected scrypt work factors; actual target-host feasibility output is recorded above.
- [SQLite conditional UPSERT](https://www.sqlite.org/lang_upsert.html) and [safe connection/open flags](https://www.sqlite.org/c3ref/open.html).

Jordain reported Stanley's approval of the concrete schema, topics, account gate, SQLite dependency and policy defaults before implementation.
