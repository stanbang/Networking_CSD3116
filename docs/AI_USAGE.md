# AI usage

## 2026-11-03 — P0 environment inspection (blocked)
User requested reading CODEX_GOAL.md, exploring this project, and starting multiple agents. Orchestrator read AGENTS.md, CODEX_GOAL.md and docs/PROMPT.md; spawned three real builder/reviewer/tester agents. Agents inspected repository and Ubuntu dependency/display/security prerequisites independently. Generated PROGRESS.md, REVIEW_LOG.md and TEST_REPORT.md; no game implementation generated, installed or claimed working.

Validation: actual read-only Windows/WSL commands, git status, installed-tool/package probes and source review. TEST_REPORT.md contains raw evidence. Missing dependencies prevent baseline compilation and two-client GUI verification. No P1 work started; security option remains pending installation. Human package installation is required by the project instructions. Baseline and third-party reference files left unchanged; no commits or pushes.

P0 resumed after human-installed Ubuntu dependencies. Builder actually compiled the baseline; reviewer independently confirmed versions and installed DDS Security plugins. Option A selected. Tester is capturing runtime evidence; no new game source yet.

P0 complete: tester generated and ran an Xvfb/xdotool two-client harness plus pixel assertions, with real movement replicated in both windows. Reviewer independently inspected baseline ELF protections and evidence. P1 implementation now authorized by orchestrator against recorded acceptance tests.

P1 complete after two loops. Builder generated v2 IDL, networking, CLI, headless and connection-view modules, baseline binding isolation, validators and unit tests. Reviewer independently found lease unsigned-underflow race, added stricter handshake/snapshot validators and tests; builder fixed lease arithmetic with regression tests. Tester independently executed lifecycle/fault script before and after fixes. Final actual output in TEST_REPORT. No crypto/gameplay claim. P2 now starts; key path chosen on Linux because Windows drvfs chmod probe stayed777.

## 2026-11-03 — P2 partial, three-loop checkpoint
Builder generated native security configuration, signed certificate/policy tooling, owner-only Linux temporary key storage, minimal identity export, secret path checker, rate buckets, audit chain and native diagnostic filtering. Tester generated black-box security and audit verifiers plus test-only postcrypto sendmsg/EVP instrumentation. Reviewer identified availability, log-rate and credential-distribution concerns. Orchestrator wrote THREAT_MODEL.md, corrected .gitattributes and actual-state README.

All three real agents then hit runtime usage limits. Following the explicit goal fallback, root acted as sequential builder/reviewer/tester with fresh code reads. Reproduced non-reader timeout, changed reliable Response history to latest-only, corrected harness cleanup, rebuilt warning-free, ran unit/security/lifecycle tests and minimal-key export checks. Real output is pasted in TEST_REPORT; root role passes are not independent-agent passes.

P2 security harness now exits0, including actual ciphertext mutation/native GCM failure and audit chain tamper/truncation tests. Final review identified no-cert server-audit gap not asserted by harness. Stopped at the goal's maximum three loops; P2 remains incomplete, P3–P6 not started. No complete-game or definition-of-done claim. No private key contents, git commits/pushes, sudo or installs performed by agents.

2026-11-04 resumed P3: three real agents available again. Builder generated deterministic fixed-pool game simulation, bounded snapshot transport/validation, client interpolation and GLFW renderer; tester authored independent collision/physics fixtures and live five-bot/GUI harnesses. Reviewer independently resolved no-cert native discovery audit gap and reviewed safe frame/role/indices. Orchestrator generated tools/play_local.sh plus human PLAYTEST.md; launcher syntax and actual five-second two-client game run exited0. Private credentials remained in Linux owner-only storage; no key contents/commits/pushes/sudo. Phase acceptance pending final security/GUI review, not claimed completed here.

P2 resumed accepted after real native unauthenticated-discovery trace was integrated and independently tested in the actual game. Tester strengthened every negative with newaudit-record/no-admission assertions where observable, added Leaveflood and actual velocity/tick bounds, verified postcrypto bitflip/nativeGCM/productiondecodeaudit, and audit mutation/truncation. Full command output in TEST_REPORT. P3 latestGUI rerun and fivebotphysics passed; controlled networkgameover/restart scenario pending. No claim externalpacket sniff or humanfeel/LANplay tested.

P3 accepted2026-11-04 after independent realfiveplayer andtwoGUI runs plus explicit controllednetwork gameover/restart. Builder/reviewer/tester roles allreal; no AIgenerated gameplay screenshot. Humanfeel/LAN/externalsniff remain humanchecks. P4 memory/hardening verification now starts; no sanitizerpass implied yet.

P4 checkpoint2026-11-04: real builder/reviewer implemented and independently tested fail-closed hardening checker; reviewer guarded diagnostic-throttle timestamp underflow. Real tester ran actual120s ASan/UBSan5clientchurn and passiveAES256GCMserveregress observation. After all three agents hit quota, root performed sequential roles: fresh reads, coherent allmodeunit/ELF checks, nativeASan auditprobe, actual unsuppressedTSan session (FAIL libraryrace reports), Valgrind6process run (zeroheap retained but FAIL clientuninitialisedcrypto-send diagnostics), final cleanpublicsourcecopy build (PASS). Exact logs retained; no false fullverification claim. Stop at actual missingCMake tool boundary; human apt command given. No sudo/install/globalASLR or gitcommits/pushes, privatecontents or immutablefilechanges. Human feel/LAN/externalsniff and P5/P6 remainpending.

2026-11-04 checkpoint: user explicitly paused LAN setup and requested a local commit plus remaining-work list. Root reviewed changes and generated docs/TODO.md from actual evidence. No new LAN test, host network/firewall change, missing-tool installation, push or memory-clean claim. Explicit user commit authorization overrides the default human-commit rule for this one checkpoint. Staging/commit verification excludes generated artifacts/private key contents.


## Local main integration,4November2026

See INTEGRATION.md. Source2cad2e1 fast-forwarded after preserving localP0 history/bytes; nativeA/IDL/P1 integration authorized by user. Independent agents compiled/reviewed/ran initial failedP1 test, then usage limits caused SUBAGENTS_UNAVAILABLE sequential fallback. Fallback fixed audit overwriting with a real preservation regression and strengthened fresh-output/join-aware fault tests; actual diff reread before checks. Units passed; networkingrun3 passed but original shutdownhang stays unresolved. No sudo/commit/push, no privatekeys inrepo. P4 HIGHs and P5/P6 work remain.

Sequential fallback added fresh-output GUI harness support and raising before capture, preserving earlier artifacts. Actual secure GUI smoke failed with peak1/timeouts2 and no movement/fire; images inspected and failure retained. Secure launcher headless startup/duplicate refusal/live stop passed with no remaining game processes. No successful GUI manual session is claimed. Work stops at bounded-loop gate pending explicit additional-cycle authorization.


## Paused teammate checkpoint
User explicitly paused goal dueusage and authorized handover+commit/push. HANDOVER.md is current; selected actual nonsecret output copied under docs/evidence/2026-11-04-handover/ because build/ is ignored. Native ACK diagnostic measured requested1e9ns elapsed1000082330ns result-10; this is not a waitset overshoot. Latest active GUI has timeouts0/invalid0/freshconnected state, but overall network and GUI remain FAIL during ACK shutdown. No game tests/repairs ran afterpause, no private material included, no completion claim.

## Resumed P1 diagnosis, 5 November 2026

User resumed the paused goal. Orchestrator verified checkpoint3dae8e3 on remote main and clean initial worktree. Real builder/reviewer/tester roles resumed. AI assistance added metadata-only before/after matched-reader and every-return ACK timing in the diagnostic shim, plus an isolated owned-child Leave probe. Strict shim compilation exited0; isolated probe exited0 with one matched reader before/after and native ACK0 in1843ns. Fresh unchanged full P1 comparison exited1 at fault-spoof shutdown, matched count1 before/after and ACK-10 after1000571819ns. These diagnostics do not change production timeout, wire, security or acceptance assertions. Full P1 remains failing and has no manual approval.

Independent tester identified an ordered audit realtime reading reversing1427507004ns between JOIN and DROP; reviewer checked the audit mutex serializes timestamp acquisition. Connection to DDS timeout is unproven. A paired-clock observer is being prepared to investigate without clock/affinity changes. Native private interceptor was rejected after actual library disassembly showed direct bound internal calls, so no ineffective interceptor was added. No new phase, private material, sudo, commit/push or completed-goal claim.

## Approved T1 fallback verification - 5 November 2026

User approval: "Approved T1". Separate review confirmed only DDS_RETCODE_TIMEOUT following successful Leave write is downgraded to an explicit warning. Write errors, other ACK errors and cleanup errors remain fatal. No security, QoS, lease, wire-format or timeout changes. The native ACK problem remains a known limitation.

Actual incremental client compile exit0; full compiler output: build/evidence/slim-t1-build.txt. Targeted native lifecycle run, no preload/thread cap: tests/p1_network.py --security-dir <private Linux bundle> --output-dir build/evidence/slim-t1-network1; exit0. Output includes PASS fresh server slot release per identity, PASS final fault slot permits replacement, SERVER_SUMMARY joins13 rejects1 leaves12 timeouts1 invalid4 stale1 peak5, and PASS lifecycle audit records36. One timeout is the intentional SIGKILL lease test. Complete output and exit file retained; this run did not emit the warning and alone did not exercise the fallback branch.

Explicit test-only public API fixture tests/force_ack_timeout.c simulates DDS_RETCODE_TIMEOUT, without probing internals. Compiled with cc -std=gnu11 -Wall -Wextra -Werror -fPIC -shared; isolated tests/ack_probe.py --dds-markers build/force_ack_timeout.so --output-dir build/evidence/slim-t1-forced-timeout exited0. Actual output: CLIENT_WARNING leave_ack_timeout: transport acknowledgement unconfirmed; server slot release requires independent verification; CLIENT_EXIT; server_leave_observed true; audit_leave_observed true; client_cleanup_exit0; server_cleanup_exit0; status PASS. This establishes application handling of an injected timeout, not native ACK reliability. Full output: build/evidence/slim-t1-forced-timeout-output.txt.

T1 approved fallback implemented and targeted checks passed; T2 reproduction pending. Existing unsuppressed dependency findings and all historical failures remain disclosed.


## T2/T3 slim checkpoint - 5 November 2026

T2 fresh clone plus current working patch: build/unit/hardening exit0; headless124.539seconds after both admissions, joins2/leaves2/timeouts0/invalid0, audit verified; strict GUI exit0, both new final screenshots inspected once with connected HUDs, ships and asteroids. Not a clean published final revision. Complete nonsecret output copied into docs/evidence/2026-11-05-slim-finish/. T3 docs/EVIDENCE.md uses existing results only, labels historical P2 results, records missing idle CPU/profiler data and all excluded work as limitations. Exact course perf commands prepared with explicit missing-binary prerequisite. STOP at T3 approval gate; no T4 capture work or T5 packaging yet. Native ACK reliability remains unproven despite approved application fallback. No new sanitizer runs or benchmarks.


## T3 approved; explicit manual guide and capture preparation - 5 November 2026

User: "approved, but with a catch. show me how to start the game, with explicit instructions for me to test and verify. tell me exactly what you did , instructions to build and run the program." Read current launch scripts and actual key bindings. Added RUN_AND_VERIFY.md with exact Ubuntu build, credential, server/client terminal commands, observable human checklist, targeted lifecycle reproduction, managed shortcut/stop distinction and source-ZIP instructions. README stale failure-only banners replaced with current evidence and limitations; no application changes this turn.

T4 one targeted capture-helper update: old P0 snaplen256/count10000 replaced by snaplen65535/count4000, accurate secure-game guidance; original ownership/mode/no-overwrite safeguards preserved. Offline tests: PASS bash syntax; PASS invalid duration/path rejected before capture; PASS offline IPv4 RTPS filter: 1 matching packet, complete2042bytes, non-RTPS excluded; exit0. This is synthetic fixture evidence only, not encryption or privileged capture evidence. Full output docs/evidence/2026-11-05-slim-finish/slim-t4-capture-check.txt. Human live capture and subsequent unprivileged analysis remain required. No full suite, benchmark, sanitizer or GUI rerun this turn.


## Slim final-documentation reconciliation - 5 November 2026

T3 approval is recorded. README current limitations and known team roster, EVIDENCE status, AI-use record and RUN_AND_VERIFY handover are reconciled. Source-ZIP instructions explicitly require human review/commit, forbidden filename check and archiving the reviewed revision; no archive has been generated or submitted. Exact requested live capture path /home/stanley/.local/state/dds-asteroids/captures/secure-manual-20261105-01.pcap was checked: Exists False; Bytes not available. No new live capture, game run, build or test rerun. T4 analysis cannot proceed without the actual human-generated file/path. T5 documentation/instructions are prepared, subject to adding the real capture outcome and final evidence audit. No full-goal completion claim.


## User-requested automatic launcher and lecturer checklist test - 5 November 2026

See REQUIREMENTS_CHECK.md and docs/evidence/2026-11-05-requirements/. Real play_local.sh --duration45 under isolated software-GL Xvfb passed two admissions/windows, real key input, fresh shared state, first-client Leave and session exit0: joins2 leaves2 timeouts0 invalid0 peak2 moves1627 shots14 snapshots310. Both new final screenshots inspected once. Actual WSL and OpenGL/GLFW/DDS linkage confirmed; no remaining game/Xvfb process rows.

Targeted authentication first attempt FAILED due to my identity2 fixture selection: rogue_client2.crt missing, so no server auth-warning audit. Preserved and reported. One test-setup correction selected generated rogue identity6: wrong-ca and no-cert rejected exit1 while a legitimate client stayed alive; no extra slots, auth_failures6, server audit verified, test exit0. No product/security/assertion change.

Approval is limited to tested launcher/local multiplayer/certificate rejection. Cross-machine operation and external encryption capture remain unverified; cached-glyph/condition-wait concepts are confirmed but speedup/idleCPU is unmeasured. Known ACK/sanitizer limitations retained. No full-suite or sanitizer/benchmark rerun and no agent sudo. User's full requirement list is not yet approved.


## Teammate takeover and authorized publication - 5 November 2026

Stanley requested a concrete MD handover, proposed Jordain account/login/high-score/remember-user responsibilities, tests/gates and ZIP guidance, plus push of completed files. Codex authored TEAMMATE_HANDOVER_2026-11-05.md from existing source/results; proposed contributions are explicitly unimplemented and must not be claimed as done. No game/account code added, no private credentials/capture/database published, no tests rerun solely for documentation. Publication is explicitly authorized despite the default AGENTS no-commit/push rule. The runtime-unverified local clock-witness modification is excluded from the completed-work commit and preserved locally. Commit/push result must be verified separately before claiming published.

## Jordain pulled-code review, 4 November 2026
Three real agents reviewed commit 3dae8e3, independently forced the release build/unit/hardening checks and audited remaining deliverables. Root ran one fresh secure P1 lifecycle and one strict two-GUI test; both exited0, screenshots inspected and cleanup verified. CURRENT_REVIEW.md and TODO.md now distinguish these local passes from preserved Stanley-host ACK/server-hang failures and unresolved P4/P5/P6 work. Selected actual output retained under docs/evidence/2026-11-04-jordain-review. No production source changes, package installation, private key contents, commit or push.

## Safe pull and integration — 5 November 2026

User requested pulling teammate changes. Root verified an external backup of 22 changed/new files, stashed them, fast-forwarded main to `480227c`, reapplied the retained stash and resolved five conflicts. Reconciled the approved timeout-only Leave fallback with local padding protection/diagnostics, full-packet default capture with optional modes, and both documentation histories. All 17 original files outside those five conflict resolutions were verified unchanged after newline normalization. Build/units/hardening, updated P1 and injected timeout regression all exited 0; server slot release and audit Leave were observed independently. Original implementation work remains paused; no new account feature, package install, commit, push or secret publication occurred. Backup/stash retained; working changes unstaged. Evidence: `build/evidence/pull-480227c-20261105T030706Z/`.

## Jordain J0 account-feature design — 5 November 2026

User supplied Stanley's teammate handover and said he was assigned this work. Root and three real agents read the actual code, compared attachment/repository text, verified absent account features and SQLite C dependency, and prepared short approval summary, detailed design, bounded IDL/unapplied patch and SQL schema. Review corrected liveness/cancellation/replay, cached-secret/revocation ordering, score mailbox concurrency and shared-certificate trust boundaries; first version proposes one account per existing certified identity. No new production source/IDL/topic/policy change or dependency installation. Draft syntax/SQL checks passed and existing OpenSSL scrypt candidate was benchmarked using random temporary inputs with no values printed. Public nonsecret feasibility outputs are in docs/evidence/2026-11-05-accounts-j0; scratch code/logs in build/account-j0. Features remain unimplemented and await the handover's Stanley approval of concrete schema/policy/SQLite before J1. Excluded native investigation was not resumed; no commit/push or message to Stanley was sent.

## Jordain J1–J4 implementation and verification — 5 November 2026

Jordain confirmed Stanley's design approval and installed SQLite headers in Ubuntu. Codex and three real agents implemented and reviewed the account IDL and permissions, single storage worker and SQLite store, account gate and score capture, GLFW login and profile/token lifecycle. Codex also adapted the launchers and tests, added private test credentials and account regressions, and inspected the masked login and two in-game screenshots.

The fresh `-Werror` build, units, hardening, live account restart and revocation, native P1/P2 regressions, controlled game-over and strict graphical tests passed. [Actual output](TEST_REPORT.md). [Open manual checks](TODO.md).

## 2026-11-05 account and offline integration
Codex generated a public-library/header offline SDK, bootstrap scripts and private test drivers, then ported the approved30second reservation to Jordain account authentication. Native build passed; units/hardening and account regression exited0. A failed crash-test startup was traced to its working directory and corrected in the harness. Authenticated reclaim and whole-round shutdown passed; real score260 to660, lives3 preserved, distance166663 to306794. Earlier fresh-ZIP offline compile/provision/headless/GUI passed before reconnect changes. Final integrated fresh-ZIP GUI and120second soak are running; do not claim them passed yet. No private credentials were packaged and no packages installed by runtime.

## Final integrated offline result
# Final runtime integration,2026-11-05

The selected account/password/SQLite implementation is integrated with the30second crash/rejoin reservation. The verified runtime is now in Networking_CSD3116; every previous root file except preserved .git was moved into root-before-account-promotion in this archive. Full development source, tests and docs remain in Networking_CSD3116_accounts_c50af1e. The tested ZIP contains runtime source, launch/build/provision scripts, generated DDS source, public dependency headers/libraries, required license records and RUN_OFFLINE.md. No private keys, profiles, databases, captures, logs or Git history are in the ZIP.

Ubuntu24.04 x86_64 under WSL2/WSLg with an existing GCC C toolchain is required. Run bash run.sh from the extracted DDS_Asteroids directory. No packages or downloads are invoked. Build-only: bash tools/build_portable.sh.

Actual final outputs (offline-final-verification.txt, exit0):
PASS bundled Python modules sqlite3/ssl/ctypes/fcntl/json
PASS fresh ZIP extraction: offline compile, fresh certificate provisioning, authenticated two-client game and exit0
PASS fresh ZIP bundled software OpenGL: two visible GLFW windows, registration, authenticated joins, keyboard input, screenshot, exit0
PASS offline extracted two-minute two-client soak: exit0 joins2 timeouts0 invalid0

The fresh-extraction test blocked apt,apt-get,sudo,make,pkg-config,idlc. New screenshot gui-offline.png inspected once: both players connected, separate player1/player2 HUD, asteroids, ships, firing and distinct account names.

Native account regression and hardening: account-final-regression.txt, exit0. PIE/NX/RELRO/BIND_NOW/canary references for both binaries; protocol/sim/event/account-store unit checks pass. Certificate-only admission, forged account session, unknown account and rapid retry rejected. Server-earned score420 persisted. Leaderboard, remembered-token rotation/replay rejection/certificate binding/logout/forget all passed.

Reconnect: account-reconnect-test-02.txt exit0. Same-account/certificate crash reclaim kept player1. Score260->660, lives3, cumulative distance166663->306794. On second crash, entire round/server/surviving client ended with exit0 after31.981seconds including final display. Protocol version4 prevents silent compatibility with older game messages. Countdown boundary30 accepted/31 rejected (countdown-boundary-test.txt exit0).

Known limitations remain: external packet capture missing (previous human capture had0packets), no two-machine run, no controlled idleCPU before/after or full before/after performance measurements. Historical TSan/Valgrind dependency findings remain open, not rerun or cleared. Leave ACK warning fallback remains previously approved. During the terminal round-end interval, already in-flight inputs can produce session-drop audit records. WSLg desktop human acceptance has not been substituted by Xvfb automation. Slides/video/peer evaluation remain team deliverables. These are not claims of full rubric completion.

No new commit or push was performed. The final ZIP is local, not a published clean Git revision.


## Windows double-click launcher fix
The reported failure was opening run.sh directly from Windows File Explorer. The working-folder offline build and full run.sh headless session both exited0. Added RunGame.cmd, which explicitly invokes the default Ubuntu WSL distribution in the script directory and pauses after errors. Added visible server/client compilation progress. No C, IDL, account, security or SDK library changes.

Actual Windows entry-point test: RunGame.cmd --headless --duration8, WINDOWS_LAUNCH_EXIT=0:
Starting DDS Asteroids in WSL. The first compilation can take about a minute.
[1/2] Compiling server...
[2/2] Compiling client...
Offline source build complete. No packages installed.
Local session started. Logs: build/local-session.DJjRorpS server=9485 client1=9500 client2=9527

Updated distribution: DDS_Asteroids_offline_source_windows_launcher.zip,94314441bytes,1102 unique files, SHA2563060b8f082fcf858cbcec252e69e8374c898bfebe918700825eb73a926451484. Every packaged file verified against current runtime. No private keys/databases/profiles/captures included. The old ZIP is retained as historical evidence; distribute the Windows-launcher ZIP. Windows usage: extract the entire ZIP and double-click RunGame.cmd, not run.sh. Ubuntu terminal usage remains bash run.sh. Test mode above is headless; the earlier graphical registration/input evidence remains in offline-final-verification.txt. External packet-capture evidence remains pending.

## Password minimum and clearer wording,2026-11-05
User requested minimum6letters and simpler wording. Passwords now accept6-128 printable characters with no uppercase/number/symbol requirement. Both network/client validation and the server store enforce the same lower bound. Existing password hashes, accounts, tokens, database schema, TLS/DDS security and wire format unchanged.

Changed account form to Sign in or create an account, Create account and Stay signed in. Removed internal jargon about certificates, authentication and password bytes from the login form. Field-specific username/password format errors and plain generic sign-in failure messages preserve account-enumeration protection.

Actual outputs:
PASSWORD_TEST_EXIT=0
PASS password minimum: five rejected, six/seven letters accepted
PASS six-letter password registration/login; five-letter password rejected
PASS existing store score/token/binding/path safety regressions
PASSWORD_BUILD_EXIT=0 (bundled offline build, no packages installed)
PASSWORD_GUI_EXIT=0
PASS GUI: five-letter password blocked; adding sixth letter registers player1; six-letter player2 registration accepted
PASS both graphical clients joined; password text remains masked; screenshots saved for visual inspection

password-six-validation.png and password-six-connected.png inspected once: clear, fitting form text, five masked password characters and format error, second player's six-character hint; final screenshot both authenticated players in game. No full-suite/soak or sanitizer rerun. Distribute DDS_Asteroids_offline_source_password6.zip; it retains Windows and reconnect launchers. Restart with RunGame.cmd to load the change. External packet capture remains pending.

## Multiple accounts per player,2026-11-05
User explicitly approved allowing any number of registered accounts per player window. Removed the per-identity unique constraint and global six-account capacity trigger. Usernames remain globally unique. Accounts/tokens stay certificate-identity bound; one active client per participant identity and the gameplay player cap remain unchanged. Passwords still allow six digits with no composition requirement. Registration errors now suggest a different username, not an account-per-window limit.

Schema version2 is created for new stores. Existing version1 stores are validated under the exclusive lock and upgraded transactionally: create/copy the accounts table preserving all IDs/columns, remove old restrictions, validate integrity/foreign keys, commit or roll back, restore foreign-key enforcement. Password hashing, existing password values, score provenance and tokens are preserved.

MULTI_STORE_EXIT=0 (multi-account-store-test.txt):
PASS legacy migration preserves every account/score/token field byte-for-byte
PASS twelve accounts on one player beyond old global cap; login/reopen/unique usernames and certificate binding
PASS existing six-character, score persistence, verifier material, token rotation/expiry/revocation, unsafe-path and exclusive-lock checks
MULTI_BUILD_EXIT=0 (multi-account-build.txt): bundled offline build of client/server in development worktree; no packages installed.
REAL_MIGRATION_EXIT=0 (real-account-migration-test.txt): migrated a private copy of the user's exact original-schema database1->2; both accounts and every stored account/password/score/token value identical; original and backup untouched.

Private consistent backup: /home/stanley/.local/state/dds-asteroids/backups/accounts-before-multi-3y42hbou.sqlite3 (mode0600 under mode0700 backups). No private data is included in the source ZIP. The live user's old server was left running to protect their session; the primary runtime contains updated source. Close the old launcher terminal, then double-click RunGame.cmd to compile/restart and upgrade the live store. Do not run the older game against the upgraded schema. Historical artifacts are retained, not the current distribution.

Distribute DDS_Asteroids_offline_source_multi_accounts.zip. No new DDS wire/security mechanism change, full-suite/soak/sanitizer rerun or live-session interference. Account registration/login and migration were verified through actual store APIs; no new concurrent DDS network test was run alongside the user's live server. External packet capture remains pending.
