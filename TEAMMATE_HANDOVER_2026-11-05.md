# Teammate takeover: DDS Asteroids

**Prepared for Jordain Ng by Stanley Lur with declared Codex assistance. 5 November 2026, Singapore.**

This is the latest takeover brief. Build on the existing game; do not restart from the lecturer skeleton. Older HANDOVER/PROGRESS entries and prompts are historical when they conflict with this brief or Stanley's later direct instructions. This document assigns proposed work, not completed human contributions.

## 1. Goal, team and honest current position

Repository: https://github.com/stanbang/Networking_CSD3116. Team: Stanley Lur (leader), Jordain Ng; no team name. Supplied assignment brief lists Friday6November2026; check Moodle yourself. Required stack: C, Ubuntu/WSL, OpenGL, GLFW, Cyclone DDS. macOS is an allowed alternative, not an extra required port. Minimum two players; real-time interactive distributed secure simulation; demonstrate optimization/threading and client authentication/encrypted DDS. Source ZIP, presentation PDF, real team-narrated MP4 (<=5minutes/200MB), AI disclosure and peer evaluation are separate deliverables.

**Current game runs locally. Full rubric/security/distributed approval has not been granted.** New account login, persistent high scores and remember-user functionality are requested work and ARE NOT IMPLEMENTED. Existing certificate admission is not a username/password login. Do not show a slide claiming these new features are done.

Stanley requested publishing completed source/docs/tests. The last previously published checkpoint was3dae8e3; later tested changes are being published with this handover. Verify the pushed checkpoint hash from the chat/GitHub. One local change to tests/gui_runtime_diagnostics.py adding an opt-in clock witness is unverified at runtime and intentionally excluded from the completed-work commit. The published version retains its earlier behavior. That unfinished diagnostic is out of scope; do not spend the finish budget on it.

## 2. Already implemented: preserve these

- Server-authoritative co-op Asteroids at60Hz, up to5 active players, bounded ships/bullets/asteroids, movement/inertia, firing, asteroid splitting, score/lives, respawn, waves and game-over/restart code. GLFW/OpenGL clients render authoritative snapshots with interpolation.
- Native DDS Security Option A: CA-signed identities, signed governance and per-identity role permissions, encrypted RTPS/metadata/data settings, validation, session/sequence checks, rate limiting and chained server audit. Six provisioned certificate identities, five gameplay slots.
- Explicit Leave and3-second crash lease; server derives authority from the fixed authenticated reader channel, not a claimed player-id field. Existing Ast IDL stays in place.
- Bounded listener notification queue avoids the observed long application waitset stalls. Callbacks only mark readiness; network work happens outside that queue lock. Main/render and DDS worker responsibilities remain separate.
- Cached text glyph geometry changes only when text changes; fixed pools/bounds and short state-copy locks. These are applied concepts, not proof of a measured speedup.
- Audit outputs refuse existing/symlink targets. Output directories are fresh; historical failures are retained. Scoped play/stop tools exist. Private credentials live outside the source tree.
- **Approved limitation:** only DDS_RETCODE_TIMEOUT after a successful Leave write logs CLIENT_WARNING and allows normal exit0. Other write/ACK/cleanup errors fail. This is NOT a native ACK repair. Tests independently require fresh server release evidence and successful replacement joins.

Key navigation: src/client/net_client.c (worker/admission/Leave); src/server/net_server.c (reader-derived authority, admission/input/Leave); src/server/main.c (simulation/lease); src/common/event_queue.* (bounded notifications); src/common/protocol.* and config.h (validation/caps); src/common/security.c (native setup); src/common/audit_log.*; src/common/sim.*; src/client/render.c (drawing/input/cache); idl/messages.idl; tools/gen_certs.sh and config/cyclonedds.xml.

## 3. Verified results and failures: do not overstate

| Check | Actual result / evidence |
|---|---|
| Fresh GitHub clone plus uncommitted source patch | Build, make test, hardening exit0; this was not yet a clean published final revision. docs/evidence/2026-11-05-slim-finish/ |
| Two-client headless soak |124.539seconds after both admissions, joins2/leaves2/timeouts0/invalid0, continuing own-state streams and audit verified |
| Strict GUI | Exit0, real key input, >=10seconds state progress, both new final screenshots visually inspected |
| Actual automatic launcher | tools/play_local.sh --duration45: two real windows/IDs, moves1627/shots14/snapshots310, joins2/leaves2/timeouts0/invalid0; first-client Leave ends managed session exit0 |
| T1 lifecycle | Exit0:13joins/12leaves/1intentional SIGKILL lease timeout/peak5; fresh server release after each graceful stop and final replacement |
| Injected ACK timeout | Test-only public API fixture exercised warning/exit0; real server Leave/audit still observed. Does not prove transport ACK success |
| Fresh client authentication | Wrong-CA identity6 and no-cert rejected exit1; legitimate peer survives, no extra slot, audit verified, auth_failures6 |
| Failed authentication attempt retained | My first test selected identity2, but generator supplies rogue_client6 only; missing certificate caused local failure/no server auth audit. One test-setup correction to6 passed. Not a game repair |
| Historical P2 tests | Existing no-cert/wrong-CA/rogue writer, rate limits, slow reader, GCM tamper and audit mutation results are in TEST_REPORT/EVIDENCE. Not all rerun on the latest checkpoint |
| External gameplay capture | NOT DONE. Earlier human capture had0packets; no encryption proof. Updated helper passes syntax/invalid-argument/offline2042-byte RTPS fixture checks only |
| Performance | Controlled idle CPU before/after and benchmark results absent. Course perf commands are prepared, not executed |
| Memory/thread tools | Historical unsuppressed TSan native race/mutex-lifetime and Valgrind uninitialized crypto-send findings remain open. Not declared false positives |

Read docs/REQUIREMENTS_CHECK.md for the exact lecturer-requirement matrix, docs/EVIDENCE.md for slide-ready evidence/limits, and docs/RUN_AND_VERIFY.md for complete human commands. Portable outputs and selected screenshots are in docs/evidence/2026-11-05-*/. Original large runtime logs are ignored under build/ and will not come with a fresh clone.

## 4. First-day setup: do this before new features

Clone the published checkpoint into your own Ubuntu/WSL workspace. Do not copy Stanley's private CA fixture, /home/stanley paths or generated key directory. The setup helper may require human installation; the assistant must not run sudo.

```bash
git clone https://github.com/stanbang/Networking_CSD3116.git
cd Networking_CSD3116
make -j2
make test
make check-hardening
bash tools/play_local.sh
```

The launcher builds, generates/reuses private Linux credentials, starts server first and clients sequentially. Click each window: Up thrust; Left/Right rotate; Space fire; R restart after game-over; Esc exit. Both must show CONNECTED / DDS SECURE. Closing either client ends the managed session. Ctrl+C in the launching terminal stops its owned children. Use RUN_AND_VERIFY's separate-terminal method for client leave/rejoin without stopping the server. Never launch overlapping test servers in domain42.

For explicit repeatable testing, create a NEW private bundle:

```bash
umask 077
mkdir -p "$HOME/.local/state/dds-asteroids"
BUNDLE="$HOME/.local/state/dds-asteroids/credentials-$(date +%Y%m%d-%H%M%S)"
bash tools/gen_certs.sh "$BUNDLE"
```

Do not precreate BUNDLE itself; generator expects a new directory. Require directory0700, private keys0600 and no symlinks. Keep BUNDLE shell variable in your test terminal. Never put keys in /mnt/c, source ZIP, screenshots or logs. CMake was missing on Stanley's machine; it is only a prerequisite for the excluded dependency investigation, not required by the existing Makefile game build.

## 5. Jordain's proposed contribution: requested new work

### J0 - review and design approval first

- [ ] Reproduce the current launcher/build and record your machine/checkpoint and outputs. Preserve current security and gameplay.
- [ ] Agree with Stanley that "login" means real server-validated accounts, rather than merely selecting a local nickname. Recommended: retain mandatory DDS certificate authentication, add account authentication on top; a remembered nickname alone must not bypass it.
- [ ] Write a short account/storage design and concrete bounded IDL diff. Obtain Stanley's approval BEFORE applying a breaking IDL/topic/policy change. Keep the existing native DDS security mechanism; no plaintext fallback.
- [ ] Confirm new persistence dependency. Recommended SQLite for server-owned account/high-score records; if adopted, document the Ubuntu libsqlite3-dev prerequisite for the human and Makefile/pkg-config change. Do not silently add a dependency or assume it is installed.
- [ ] Define account-to-certificate binding and duplicate-login policy. Server derives certificate/channel identity; never trusts username, account id or score merely because the client sent it. Do not confuse the current six provisioned identities with arbitrary public registration.

### J1 - real login and minimal client UI

Suggested new modules: src/server/accounts.{c,h}, src/server/storage.{c,h}, src/client/login.{c,h}. Reuse render.c's existing text/input and common validation style; do not replace the game renderer/framework.

- [ ] Login screen with bounded username/password fields, focus/submit, masked password, loading/error states, logout and clear distinction between native authentication and account login.
- [ ] Specify exact username/password byte limits and case/normalization policy in protocol.h/config.h. Reject zero/oversized/unknown fields before indexing/copying. Use bounded IDL fields and protocol version/nonce/sequence correlation.
- [ ] Keep per-certificate Join/Input/Leave/Response authorization. New login/register/resume response channels need equally narrow signed permissions and governance. Review all reader/tag/event-mask caps when adding endpoints; current event queue has a fixed18-channel mask.
- [ ] Admit a gameplay slot only after native identity AND account authentication succeed. Rejected/expired login must not use a slot; the normal two-client certificate mode must not silently become unauthenticated guest access.
- [ ] Store salted password KDF output only, never plaintext or reversible passwords. Use an established library KDF; OpenSSL scrypt is a possible existing-library route, subject to checked API/cost selection. Benchmark/document cost and bounded memory on the target host; no homemade hashing or weak fallback. Constant-time secret comparison and best-effort secret clearing.
- [ ] Expensive KDF/database work must not block the60Hz simulation or DDS callbacks. Design one bounded account/storage worker with bounded requests/results and per-identity throttling, rather than a thread per login or an unbounded queue. Document added thread/queue caps and busy/error behavior.
- [ ] Add generic invalid-credentials errors and bounded login rate limits; do not leak passwords/tokens, detailed account existence or raw native data in the audit.

### J2 - persistent, authoritative high scores

- [ ] Persist account id/display name, authoritative best score, update time and schema version in the server's private Linux-home data directory. Never accept a client-submitted final score as authority.
- [ ] Identify the authoritative round-end/game-over/Leave update points before slots/sim state are cleared. Decide and document whether a departed player's current round score qualifies; support exactly-once/idempotent updates using account+round/session identity.
- [ ] Transactional max update: a lower score cannot overwrite a higher one. Handle duplicate events, concurrent finishes, restart/rejoin and storage failure without corrupting records or silently claiming persistence.
- [ ] Return a bounded top-N leaderboard and own best score through server-authorized messages; clients cannot publish leaderboard responses. Show best score/leaderboard using cached text where possible.
- [ ] Create/migrate schema safely; use prepared statements, checked return codes and private file permissions. No SQL built by concatenating usernames. Database and backups stay out of git/source ZIP. On corruption/disk error, report the failure and preserve the file instead of silently resetting scores.

### J3 - remember user securely

Suggested client module: src/client/profile.{c,h}. Profile files live in private Linux home, separate from source.

- [ ] Remember username by default only if agreed; never save the password. An explicit Remember me option may use a cryptographically random server-issued resume token.
- [ ] Keep only a hash of the token server-side, with expiry, revocation and account/certificate binding. Store the raw token client-side in an owned0600 file in an owned0700 nonsymlink directory. Never log/token-display/commit/export it.
- [ ] Resume must still use the native authenticated encrypted DDS channel. Validate server-side token/session/version/expiry; reject tokens for another account/certificate. Rotate on successful resume and revoke on logout/forget/password change as designed; define behavior for parallel instances.
- [ ] Invalid/expired/revoked token returns to login rather than granting access. Missing/corrupt profile fails safely without losing unrelated data. Provide Forget user that clears local remembered state and revokes the server token when possible.

### J4 - feature acceptance before claiming contribution complete

- [ ] Add real tests with nonzero failure exits, fresh output directories and retained outputs for every row in section6. Test account security, persistence across SERVER restart and remembered user across CLIENT restart separately.
- [ ] Run the existing lifecycle and strict GUI checks after the approved protocol changes; preserve controls, admission cap, Leave/rejoin, crash lease and native security negative tests. Update certificate generator permissions and all affected harnesses together.
- [ ] Update README, THREAT_MODEL, protocol notes, AI_USAGE, evidence, build dependencies and limitations. Include a short code-walkthrough explanation and screenshots that contain no passwords/tokens.
- [ ] Ask Stanley to manually accept login/logout, best-score persistence, remember/forget and two-player gameplay. Do not self-approve an unrun check.

**Contribution wording for the slide AFTER completion only:** "Jordain Ng: account login UI/protocol, secure account persistence, authoritative high-score storage/leaderboard, remembered-user token lifecycle, and associated tests/documentation" — replace this with actual completed modules/commits/tests and remove anything not done. Current status is planned/unimplemented. Stanley is leader and has managed the integration/review/testing checkpoint with declared AI assistance; do not present AI-generated changes as unaided human authorship. Both members must provide their own real responsibilities and narration.

## 6. Test checklist and exact existing entry points

Use fresh output names. Do short targeted checks after edits; run the agreed complete finish suite once at the end. Two-minute soak, not ten. Maximum one repair attempt per issue in slim mode, then report. Do not reopen already-reviewed screenshots; inspect new renders when produced.

| Area | Required assertions |
|---|---|
| Existing units/build | make, make test, make check-hardening; both server/client PIE/NX/RELRO/BIND_NOW/canary; no new C warnings |
| Protocol/lifecycle | Concurrent joins distinct ids, cap5, sixth rejected, fresh explicit Leave event, replacement admission, one intentional killed-client lease timeout, no extra invalid sessions |
| Authentication/security | Correct certificate+account succeeds; no/wrong certificate fails; wrong password/unknown username fail without a slot; duplicate login policy enforced; account-id spoof and role-forbidden publications fail |
| Input/memory validation | Empty/max/overlong names/passwords, invalid UTF8/policy, oversized messages, unknown enums, malicious ids/session/seq, repeated auth requests; no crash or unbounded allocation |
| Login rate/worker bounds | Flood one account/certificate, bounded queue/hash work, busy/rejection audit, legitimate players remain responsive; no secret-bearing logs |
| High scores | Server awards real score, lower score does not replace best, two accounts isolated, duplicate event idempotent, simultaneous updates correct, server restart preserves best, forged client score rejected |
| Persistence failure | Missing database creates safely; migration/corruption/unwritable directory/disk failure reported; no silent reset, SQL injection or symlink overwrite; database kept out of git/ZIP |
| Remember user | Username/token persistence across client restart, still requires certificate; expiry/wrong binding/replay/rotation/logout/revocation/forget exercised; file ownership/modes checked; no plaintext password stored |
| GUI/manual | Two login windows, masked password, focus/error/loading, distinct users, movement/fire, shared state, score display, leave/rejoin; stale WAITING state fails |
| Packet capture | Nonempty actual gameplay capture, check truncation/security submessages and plaintext exposure, record remaining visible metadata; no fake approval based solely on HUD or strings search |
| Reproducibility | Fresh published clone without keys, make/test/hardening, two-client headless and strict GUI, clean extraction of final ZIP; record exact commit |

Existing commands (no network-test overlap):

```bash
make -j2
make test
make check-hardening
python3 -B tests/p1_network.py --security-dir "$BUNDLE" --output-dir "build/p1-takeover-$(date +%Y%m%d-%H%M%S)"
env -u LD_PRELOAD -u LP_NUM_THREADS LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a -s '-screen 0 1620x650x24' python3 -B tests/p3_gui.py --security-dir "$BUNDLE" --output-dir "build/gui-takeover-$(date +%Y%m%d-%H%M%S)"
```

Additional existing test entry points: tests/p2_security.py, p3_gameplay.py, p3_gameover.py, p3_snapshot_crypto.py, verify_audit.py. Inspect each --help and its output-path behavior first: some historical harnesses use fixed build directories and must not overwrite preserved failure evidence. Do not treat test filenames as proof that all acceptance rows exist; add account/persistence coverage explicitly. p3_snapshot_crypto observes/injects native crypto diagnostically and is not an external pcap.

Existing tools for separately authorized sanitizer work: make SAN=asan; tests/p4_soak.py --security-dir "$BUNDLE" --bin-dir build/asan --duration 120; TSan and p4_valgrind.py remain excluded from the slim plan unless Stanley explicitly reopens them. Never disable ASLR, suppress warnings or label findings false positives just to get green output. No sanitizer result is claimed from an ordinary release run.

## 7. Outstanding gates and original goal tail

| Gate/task | Actual status and next action |
|---|---|
| P0 / native SecurityA | Previously approved; preserve selected native mechanism |
| Breaking IDL/topic/policy change | Still requires Stanley's concrete design approval BEFORE login schema implementation |
| Original phase gates | P1 integration was not fully manually approved under the old phase workflow. Later Slim T1/T3 approvals are narrow decisions, not automatic blanket APPROVED P1..P6 |
| Slim T1 | Approved and implemented: timeout-only warning fallback, independent slot-release/replacement assertions |
| Slim T2 | Passed fresh-clone-plus-patch checks, not yet a literal final published-commit build. Reproduce the pushed handover checkpoint on Jordain's machine |
| Slim T3 | Approved after EVIDENCE.md was delivered; missing idle/perf data remain honestly labelled |
| Slim T4 | WAITING FOR HUMAN CAPTURE. Agent must never run sudo; analyze the real pcap unprivileged |
| Slim T5 | README/known limits/AI log/handover/run guide/ZIP instructions prepared; add actual capture and new feature results, then final evidence audit |
| New account-feature gate | Agree account/certificate/storage/remember policy, approve IDL, implement, review/tests, then Stanley manual acceptance |
| Submission | Final reviewed committed source ZIP/extraction check, slides PDF, real all-member narration MP4, AI declaration in every deliverable and peer evaluation remain team work |

Human capture command in Ubuntu, during two-client play:

```bash
sudo bash tools/capture_demo.sh 60 secure-manual-20261105-01.pcap lo
```

Capture output is private under $HOME/.local/state/dds-asteroids/captures/. Use a NEW basename if it exists. Helper now selects IPv4 RTPS on any UDP port with full65535-byte snapshots, max4000packets, max120seconds, private ownership/mode and no overwrite. No IPv6/reassembly. Supply exact saved path/packet count to Stanley/Codex. Zero packets is a failed capture. Analyze without sudo; do not commit/submit the pcap or infer encryption merely from absence of readable strings.

Still-open limitations: native Leave ACK expiry, historic native waitset/shutdown anomalies, unsuppressed TSan/Valgrind dependency findings, loopback-only/no two-machine evidence,64sessions per identity per server lifetime, no automatic recovery after server restart/lease expiry, x86_64 Ubuntu plugin paths, missing controlled CPU/perf metrics. Further GUI internals, dependency probes, two-computer testing, slides/video production were excluded from the last slim agent scope; they are not magically completed. Stanley must explicitly authorize excluded engineering investigation before an agent resumes it. Login/high-score/remember-user is newly requested feature work for Jordain, not permission to ignore those limits or weaken security.

## 8. Submission ZIP: include and omit

**Include:** Makefile, README.md, src/, idl/, config/, tools/, tests/, baseline/ and third_party/ including their license notices. Include bench/ only if it contains actual submitted benchmark source, not generated results/binaries. Keep necessary build metadata and any actual LICENSE/NOTICE files. Generated DDS C bindings are rebuilt by Makefile; build/ is not source.

**Keep these documentation files for marking/defence:** docs/AI_USAGE.md (mandatory AI disclosure), THREAT_MODEL.md, EVIDENCE.md, REQUIREMENTS_CHECK.md, RUN_AND_VERIFY.md, TEST_REPORT.md, REVIEW_LOG.md, PDF_REQUIREMENTS.md, HARDENING.md, P1_PROTOCOL.md and P3_GAME.md. Include selected nonsecret docs/evidence/ text/PNG evidence referenced by them. Add new account/storage protocol/security docs and real contribution evidence. A source ZIP does not replace the separate slides PDF/video MP4 or peer evaluation.

**Optional for the source ZIP; preserve in GitHub:** this takeover file, docs/HANDOVER.md, PROGRESS.md, TODO.md, INTEGRATION.md, PHASE_APPROVALS.md, P0_RUNBOOK.md, P1_PROTOCOL_PROPOSAL.md (old proposal), docs/PROMPT.md, docs/history/, root AGENTS.md and CODEX_GOAL.md. These are planning/agent/history material, not game runtime or mandatory grading artifacts. PLAYTEST.md can be omitted if still a blank checklist; include it if it records actual team testing. Optional does not mean delete the audit history from the repo.

**Never include:** .git/, build/, generated executables/objects/shared libraries, credentials/CA fixtures/private keys, *.key, *.pem, *.pcap/*.pcapng, account databases/backups/remember tokens/password material, private runtime logs, crash/core dumps, IDE caches or supplied lecturer PDFs. Runtime accounts and scores are generated on the marker's machine; do not ship real personal data. Public XML templates/generator source stay included so a new private fixture can be generated.

Commit/review completed source first (human or explicitly authorized tool), then work from that commit. `git archive --format=zip --output=../DDS-Asteroids-source.zip HEAD` is a simple complete-public-checkpoint archive, but it includes optional tracked MD/history. If trimming it, remove only optional documentation from a separate packaging copy, never the working repo; update links or retain referenced evidence. Use RUN_AND_VERIFY.md's forbidden-file preflight, extend it to account databases/tokens and pcapng when added. Review the archive listing and contents for embedded secrets, then extract to a fresh directory, generate new private credentials outside it, and reproduce build/tests. Do not archive an old HEAD that omits new uncommitted work. No final submission archive or Moodle submission has yet been claimed.

## 9. Final handoff back to Stanley

Return your commit hash, real contribution list, changed modules/schema/dependencies, commands and exit codes, retained output paths, new GUI screenshots with no secrets, feature test matrix, known failures and exact remaining human actions. Explain login authority, password storage, score authority, token expiry/revocation, thread/queue bounds and the native ACK limitation in a short walkthrough. Both members should narrate only work they can defend. Do not write "all standards met" until the evidence actually supports it.
