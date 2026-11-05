# Threat model — DDS Asteroids

Purpose: security design and evidence map. Author: <team>, with Codex assistance. Date: 2026-11-03.

## Scope and trust boundaries

The server owns game state, ticks, admission and slot leases. Clients supply input states, never authoritative positions or scores. DDS carries discovery and game messages across an untrusted network. Client/server processes and the Linux OS holding their keys are trusted to protect those keys; an attacker may run arbitrary network peers, record/replay/change packets, or hold a legitimate client certificate. A compromised CA, server host or server private key is outside the protection provided by this design.

Six provisioned client identities permit testing a sixth connection; only five active game slots exist. Each identity uses exact Join/Input/Leave topic names. Server handlers derive the identity channel from their local reader and bind the admitted slot and session to that channel. Native signed permissions are required to stop another certificate from writing on it. Claimed player ids alone never establish authority.

Native DDS Security (Option A) is selected because the installed Cyclone DDS 0.10.4 includes auth, access-control and crypto plugins. Authentication, policy and encryption functionality still require independent tests; library presence is not a security pass. This document describes the P2 design and required evidence, not an assertion that every row has passed. Actual pass/fail output belongs in TEST_REPORT.md.

## Assets and controls

| Threat / boundary | Asset and example | Required defence | Evidence |
|---|---|---|---|
| Passive eavesdropper / UDP | Positions, inputs and session data | Native DDS crypto with encryption required by signed governance | Actual encrypted RTPS observation/capture; never infer wire secrecy from source alone |
| MITM or corruption / transport | Change a game input or snapshot | Native authenticated encryption; discard invalid protected packets | Real ciphertext corruption must be rejected; changing plaintext before dds_write is not this test |
| Unknown identity / participant discovery | Fake joins consume slots | Mutual CA-chained X.509 authentication before application delivery | Wrong-CA and no-certificate peers cannot join; distinguish native diagnostics from app counters |
| Legitimate malicious client / application | Move another player's ship | Exact per-certificate topic permissions plus reader-channel/session/id validation | Forbidden publication and claimed-other-id tests |
| Fake server / DDS publications | Inject authoritative state or join response | Only server certificate permitted to publish Response and Snapshot topics | Client attempts those publications and fails authorization; clients reject unauthenticated peers |
| Replay / transport and application | Reapply a recorded input or retired join | Native DDS protected transport plus application session and increasing input sequence; bounded join nonce history | Resend old logical sequence as new sample; verify retired nonce rejection and no slot theft |
| Join flood / availability | Fill slots or spend CPU verifying requests | Authenticate before allocation, bounded DDS queues, per-identity admission/rate limits and one active slot per identity | Flood rejection counts; legitimate identity still progresses; peak active slots <=5 |
| Input flood / simulation | Move faster by sending many inputs | Rate limit and latest input state consumed once per authoritative tick | Excess rejected/counted; simulation speed independent of send frequency when P3 exists |
| Invalid samples / memory | Zero/high ids, unknown versions, bad lengths or ticks | Validate before indexing/copy; bounded IDL sequences and fixed pools | Boundary unit tests and mutation negatives; later sanitizer evidence |
| Crashed peer / lifecycle | Slots never freed | Three-second lease based on valid inputs; explicit acknowledged Leave | SIGKILL with other four slots occupied, then exact slot reclaimed |
| Local audit modification / accountability | Hide rejection or change identity | SHA-256 record chain with a trusted external final-digest anchor | Independent verifier detects changed record; local chain alone cannot prove deletion of its suffix |
| Host memory exploitation | Corrupt state or expose secrets | Bounded operations, checked library returns, hardening, sanitizers and careful locking | Actual P4 sanitizer/hardening/Valgrind results; flags alone are not proof |

## Confidentiality limits

Encrypted game payloads do not imply anonymity. IP addresses, packet timing/size and some DDS discovery or certificate metadata may remain visible depending on the exact governance. Public certificates are not secrets. State precisely which RTPS regions were inspected. No claim of invisible identities or readable-field absence is valid without packet evidence. If unprivileged capture is unavailable, record the denied command and supply a human capture command, as permitted by CODEX_GOAL.md.

## Credentials and policy

Use separate identity and permissions CAs. Identity certificates authenticate participants; signed governance sets mandatory protection; signed per-identity permissions constrain roles and exact topics. A client cannot select arbitrary permissions to gain authority: policies are signed by the trusted permissions CA and bound to its certificate subject. Check the installed library constants and official documentation rather than guessing configuration names.

Private CA and participant keys must not enter git, submission zips, reports or console output. This repository is on a Windows drvfs mount where an actual chmod 600 probe still reported 777. Generate credentials on the Linux filesystem in a private temporary directory with owner-only directory/file modes, verify ownership and modes, and pass the directory explicitly. Runtime certificate material may disappear after reboot and must then be regenerated. Sharing a CA across machines requires a controlled secure provisioning workflow; never regenerate unrelated CAs independently and expect peers to trust one another.

Certificate lifetimes should be short (the starter workflow uses 30 days). Renew identities and re-sign matching permissions before expiry. After compromise, stop affected sessions, issue a new key/certificate and updated policy, and restart/reprovision trusted participants. Do not claim immediate revocation unless the implementation configures and tests it. Possession of the trusted server or CA key defeats the corresponding trust boundary. Do not ship CA private keys to clients.

## Replay, rate and availability limits

The current P1 admission history keeps at most 64 retired nonces per provisioned identity and fails closed after exhaustion. This bounds memory but limits reconnect availability until server restart; record the MEDIUM limitation rather than describing unlimited reconnection. Native security must not silently downgrade if plugins, credentials or policy files are absent. Bounded application queues cannot prevent link saturation, expensive unauthenticated handshake floods, or denial of service by the host/network administrator.

## Accounts, scores and remembered login

Native DDS authenticates the provisioned identity and protects its exact AccountRequest and AccountResponse topics. The server derives identity from the reader channel. A fresh challenge and boot epoch bind account messages to that connection. Join also requires its authenticated account nonce. Each provisioned identity is one security principal. Sharing a certificate exposes that identity's account channel to each holder. Game snapshots retain certificate-level visibility.

The sole storage worker runs OpenSSL scrypt with per-account random salts and checks one password at a time. Per-identity and global request budgets bound password checks. The server uses fixed per-identity request, reply and score slots. SQLite writes occur in private Linux storage. Scores come from server simulation, and the transactional maximum keeps the best committed result and its provenance. An abrupt server loss can lose the current uncommitted score.

Remember Me issues a random 32-byte token with a seven-day expiry. SQLite stores its SHA-256 digest. Resume requires the bound certificate identity and rotates the token transactionally. Logout and online Forget revoke the token. An offline Forget removes the local profile and reports that server revocation is unconfirmed. The client stores the token in a private, locked profile. A compromised OS user can access its runtime files.

See [the account protocol and storage design](ACCOUNT_FEATURE_DESIGN.md) and [the actual account test results](TEST_REPORT.md).

## Accountability and non-repudiation

Audit accepted joins, rejected requests, authentication/authorization diagnostics and rate limits with timestamp and identity information actually available at that layer. A failed handshake may provide no trustworthy identity; label it unknown rather than copying an untrusted claimed identity. Keep middleware diagnostics separate where no app sample was delivered.

A SHA-256 chain detects changed/reordered internal records only against a trusted expected digest. An attacker who can rewrite the log can recompute an unanchored chain or remove its suffix. Preserve the final digest separately when presenting evidence. Native authenticated transport and a local audit chain do not provide transferable signatures proving that a particular human sent each game input. Per-input non-repudiation is not implemented by this design; discuss that limit in slides.

## Sources and evidence locations

Design references: installed `/usr/include/dds/security/dds_security_api_defs.h` and [Cyclone DDS 0.10-family security documentation](https://cyclonedds.io/docs/cyclonedds/0.10.3/security.html). Implementation-specific checks must be confirmed against installed 0.10.4 headers and runtime. `REVIEW_LOG.md` records findings; `TEST_REPORT.md` records actual commands and output. External two-machine play, human packet inspection and human control/visual playtesting remain separate checks.
