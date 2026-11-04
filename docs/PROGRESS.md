# Paused checkpoint - latest authoritative status

Stanley explicitly paused the goal on4November2026 and requested a teammate handover plus publication. Read HANDOVER.md first. The listener repair preserves active two-client GUI state in the latest targeted run, but overall GUI/network checks still FAIL native one-second Leave ACK timeout. No further game tests/repairs after pause. Selected portable raw evidence is in docs/evidence/2026-11-04-handover/. Earlier status text below is historical. No goal-complete or submission-ready claim.

---
# Latest integration status: GUI verification blocked

See the current blocker section below. Existing build/unit/headless passes are narrower than full graphical multiplayer acceptance.

# Current integration status - 4 November 2026

User requested integrating the contributor merge and approved proceeding with P1/v2. Main2cad2e1 is now integrated with local work preserved. NativeA and P0 are approved. Existing P1-P3 code is present; do not treat it as a starter anymore. Fresh builds/unit/hardening pass; P1 run3 passed after evidence/harness fixes, while the first runtime hang remains unresolved. P4 has known OPEN HIGH findings; P5/P6 materials remain missing. See INTEGRATION.md for current evidence, schema differences, remaining requirements and phase gates. Everything below is preserved contributor history, not automatic fresh acceptance on this machine.

---
# Progress — DDS Asteroids

## P0: BLOCKED (2026-11-03)
Read AGENTS.md, CODEX_GOAL.md and docs/PROMPT.md. Three real subagents performed builder, reviewer and tester roles. Repository is a starter skeleton; src contains no implementation and config/cyclonedds.xml is absent. Defaults: co-op, 800x600, five players.

Plan and acceptance: inspect WSL Ubuntu dependencies; build unchanged baseline; demonstrate server and two clients moving squares; record security plugins and display/network status. Do not advance P1 before P0 passes. Reviewer owns REVIEW_LOG.md; tester owns TEST_REPORT.md; orchestrator owns PROGRESS.md and AI_USAGE.md.

Observed: Ubuntu 24.04.3 on WSL2 starts. gcc/make/openssl/valgrind present; pkg-config and idlc missing. DDS development/tools and OpenSSL development packages absent. WSLg variables/socket presence alone do not prove GUI rendering. No build, movement, multicast, sanitizer or security pass claimed. See TEST_REPORT.md for independent command evidence.

Blocker: CODEX_GOAL.md Environment limits says missing package/tool requires stopping with the exact apt command for the human. No sudo or installs executed. No source changes. baseline/ and third_party/ unchanged. No commits or pushes.

Human next step in Ubuntu:
```bash
sudo apt update && sudo apt install -y build-essential pkg-config git gdb valgrind openssl libssl-dev libgl1-mesa-dev libglfw3-dev cyclonedds-dev cyclonedds-tools xvfb xauth xdotool imagemagick tcpdump
```

Security decision: pending installation. No DDS security plugins currently found; because DDS is absent this does not establish that the installed distribution will lack them. Recheck after installation, prefer A if plugins exist, otherwise B.

P1–P6: pending. P0 must resume after dependency installation, including independent build and graphical verification. Existing baseline vulnerabilities are reference defects, not fixed-game claims.

Defence notes:
1. P0 checks the real target environment before implementation.
2. DDS headers and idlc are mandatory baseline build dependencies.
3. OpenSSL headers support the later authentication/encryption work.
4. Display variables cannot establish a successful graphical playtest.
5. Native DDS Security selection requires inspecting the installed libraries.


## P0 resumed — dependencies installed by human (2026-11-03)
User supplied successful Ubuntu apt installation output. Orchestrator independently ran `wsl -d Ubuntu -- pkg-config --modversion CycloneDDS glfw3 openssl` with exit 0 and output:
```text
0.10.4
3.3.10
3.0.13
```
The original missing-package blocker is cleared. Three existing real agents resumed baseline build, review, and independent two-client GUI checks. Prior blocked evidence remains historical; P0 acceptance still pending runtime verification.

### P0 resumed findings
Builder ran make baseline successfully; existing read-only baseline/third_party warnings remain documented by reviewer. Independent orchestrator and reviewer found native DDS Security libraries at /usr/lib/x86_64-linux-gnu/libddsc0debian/libdds_security_{auth,ac,crypto}.so. Choose Option A (native DDS Security), per the required decision gate; actual secure configuration/authentication/encryption are P2 acceptance, not yet passed.

Two-client GUI/movement acceptance is currently being tested. Network multicast delivery beyond this machine is not assumed.

## P0 acceptance: PASS (2026-11-03)
Baseline builds. Tester ran server and two distinct GUI clients under Xvfb, asserted pixel movement replicated in both clients, and inspected both HUDs. Initial harness window-search issue was corrected, then final run exited 0. Both clients joined once and each received 122 position samples. Position tuples in both windows: before [(251,130),(677,418)], after player1 input [(270,130),(677,418)], after player2 input [(270,130),(677,438)]. Screenshots/raw outputs retained in build/p0-test and TEST_REPORT. Reviewer confirms installed plugin gate; native DDS Security Option A chosen. Baseline source warnings and defects remain reference-only findings, not introduced changes. Loopback/local discovery works; multicast across machines is still unverified.

## P1 plan and acceptance (in progress)
Builder implements protocol v2 and networking foundation: bounded/versioned messages, nonce-addressed joins, checked IDs and return values, five-slot admission, explicit leave and crash timeout, finite server CLI, headless bot clients and machine-readable summaries. Preserve original IDL separately for make baseline without changing baseline/. DDS networking uses explicit QoS and blocking waitsets, no busy spins. Keep pure logic/validation independently testable. Native security is deliberately a P2 acceptance; no secure-game claim during P1.

Acceptance: clean hardened game build and make baseline; make test with malicious/boundary validator cases; independent concurrent join test yields distinct slots; five clients accepted and sixth refused; explicit leave and SIGKILL timeout permit rejoin; stale/invalid messages rejected without OOB/crash; finite CLI commands exit with useful summaries. Reviewer audits actual diff for defects 1–3,7,8,12 and records findings; tester records raw outputs. Maximum three review/fix/test loops, then stop for unresolved blocker.

P1 verification loop 1: builder make -j2, make test and make baseline passed; new implementation warning-free. Independent tester integration exited 0 (joins=12, rejects=1, leaves=11, timeouts=1, invalid=76, stale=18, peak=5, inputs=488), including per-fault events and exact killed-slot reclaim. Independent unit target passed. Awaiting completed reviewer findings/fixes before P1 acceptance. No native security or game-simulation pass claimed yet.

P1 review loop 1 found a HIGH lease-timing race: main sampled time before taking shared lock, permitting unsigned underflow if worker refreshed the slot meanwhile. Builder added guarded elapsed-time logic and deterministic regression test; reviewer strengthened join/response and snapshot validators. Loop 2 independent runtime regression is in progress. Initial runtime pass is retained as initial evidence, not final acceptance.

## P1 acceptance: PASS after loop 2
Reviewer reports zero open HIGH/CRITICAL in new P1 code. Independent final python3 tests/p1_network.py && make test exit 0: joins=12 rejects=1 leaves=11 timeouts=1 invalid=76 stale=18 peak=5 inputs=487. Exact killed-client slot reclaimed; each malicious input reason observed. New implementation warning-free; baseline builds with its isolated original IDL. Reviewer deterministic lease/validator tests pass. Remaining MEDIUM: bounded retired-nonce history accepts at most 64 sessions per identity per server lifetime; fail-closed limit documented in P1_PROTOCOL.md. Existing hardening-verifier flaw is queued for P4. P1 remains explicitly insecure until P2.

Defence notes: nonce echo addresses responses; local reader channel determines prospective identity; version/ID/session/sequence validators run before indexing; monotonic guarded lease arithmetic handles concurrent refresh; network I/O occurs outside shared-state locks.

## P2 plan and acceptance (in progress)
Choose native DDS Security A using installed security property constants and official 0.10-family docs. Builder adds mutual X.509 participant authentication, signed governance and exact per-identity permissions, encrypted payload/submessage configuration, server-only authoritative publications, bounded rate limiting, SHA-256 chained audit records, security fault modes and fail-closed secure defaults. No insecure fallback on missing files/config/plugins. Orchestrator writes threat model. Reviewer audits actual policy, API usage, state binding and no insecure downgrade. Tester independently validates secure lifecycle and each security negative, captures genuine middleware versus application rejection evidence and verifies audit chain.

Key environment constraint verified by orchestrator: drvfs mount has no metadata; chmod600 on build/p0-test/mode-check.tmp still stat777. Generate keys in private Linux /tmp mktemp directory (0700), verify keys600 and ownership; accept explicit --security-dir. Do not put private keys on the Windows mount or in logs/repo. Document lifetime/regeneration; no sudo required.

Acceptance: secure make/test pass; two to five authenticated clients work and sixth cap enforced; wrong CA/no cert/identity spoof/replay/real ciphertext tamper/flood/malformed input reject as specified without crash; exact-role authorization prevents rogue server writes; private key extension tracked check empty; hash-chain verifier detects changed records. Native security rejections may occur in middleware rather than application and must be evidenced honestly. A pre-encryption application mutation is not an AEAD-tag-failure test. Packet capture, if unprivileged capture unavailable, is explicitly a human command as allowed by goal tester item6; no invented sniff evidence. Max three review/fix/test loops; stop for unresolved real blocker.

P2 verification loop 1: native secure client joined/input/left successfully. Independent secure lifecycle passed (joins12 rejects1 leaves11 timeouts1 invalid56 stale12 peak5 inputs553). WrongCA/no-cert/forbidden server publication and rate tests showed rejection. Tester-owned postcrypto sendmsg shim actually transmitted a one-bit-mutated SEC_BODY packet (336 bytes, offset52, send_result336) and native receiver timed out corrupted peer. Initial harness expected clean leave while it was still corrupting packets, so harness disarm handling is being corrected. Reviewer found response-write timeout server-fatal behavior and unthrottled Leave logging; fixes and regression pending. Native silent crypto drop initially lacked production audit rejection; builder is adding verified0.10.4 failed-decode trace filtering rather than inventing app counters. No P2 final acceptance yet.

## P2 checkpoint: INCOMPLETE — stop at required three-loop cap

Loop1: real agents implemented native security; initial tester negatives/flood/ciphertext tests ran, harness Leave expectation failed and reviewer found response availability/Leave flood issues.
Loop2: after real agents hit runtime usage limits, root used SUBAGENTS_UNAVAILABLE sequential fallback; saved code included Leave bucket and native decrypt trace logging. Non-reading response peer hung during actual test and exceeded20s plus5s cleanup bound.
Loop3: root builder changed idempotent Response endpoints to reliable KEEP_LAST(1), fixed test cleanup; root reviewer re-read code; root tester security suite and secure five-client lifecycle passed. Final review still found no correlated server audit record for the no-cert negative and missing per-case audit assertions. Thus harness exit0 is not full P2 acceptance.

CODEX_GOAL.md says maximum3 loops per phase, then stop and report blocker. No fourth loop or P3 implementation started. Required next work: establish genuine no-cert native rejection audit event and add strict per-case audit assertions; dedicated Leave flood runtime test; then carry gameplay-dependent flood-speed and snapshot crypto checks into P3. User decision to resume/revise this phase limit is needed before another implementation loop.

Observed final security suite: slow peer60 responses survived, fresh join passed, input/join rate events passed; one-bit encrypted336-byte packet sent, native GCM failures10 and production decode/gcm-final-failed audit diagnostics observed. Audit117 records verified; mutation and truncation caught against trusted root. Final secure lifecycle: joins12 rejects1 leaves11 timeouts1 invalid55 stale13 peak5 inputs557. Full raw evidence and exact commands in TEST_REPORT.

Subagent runtime reported usage limit on all three workers. Final edits/checks were sequential root roles, explicitly not independent-agent review. Minimal client1 export verified owner-only folder/key, no CA/server private keys copied. Source checker passed. No baseline/third_party changes, no commits/pushes, no private key contents displayed.

P0 PASS; P1 PASS; P2 partial/incomplete; P3–P6 pending. Current GUI only connection status, no Asteroids simulation. README updated to match actual state. ASan/UBSan/TSan, Valgrind, full gameplay/soak, clean clone, final hardening verifier, measured optimisation and submission materials remain unverified/unimplemented.

Defence notes:
1. Native PKI authenticates participants and signed exact-topic grants bind reader channels to identities.
2. Signed governance requires native encryption; actual postcrypto corruption produced GCM rejection.
3. A latest-only reliable response prevents slow readers accumulating unbounded acknowledgement pressure.
4. Audit roots must be retained separately; an unanchored hash chain cannot prove whole-log integrity.
5. No-cert admission was denied, but its missing server audit event prevents complete security acceptance.

## 2026-11-04 — user resumed implementation; P2/P3 work authorized
User explicitly asked to implement the game as well. This resumes work after the reported phase cap with a fresh bounded review cycle; do not repeat the prior stop without new evidence. Three real agents restarted; builder confirms runtime available.

P2 resumed plan: reviewer owns native unauthenticated-peer audit observability (audit_log and discovery config), tester owns strict per-negative audit assertions and dedicated Leave flood regression. Keep real native diagnostics and unknown identity where unauthenticated. No fabricated counters or crypto. Max three fresh fix/review/test cycles for unresolved P2 issues.

P3 plan: builder implements pure server-authoritative Asteroids simulation with fixed pools (5players,128bullets,64asteroids),60Hz tick, latest input state, inertia/friction/rotation/thrust/fire, wrap, asteroid size/split/waves, scores/lives/respawn/invulnerability/gameover/restart. Extend bounded IDL snapshots and validation, publish20–30Hz outside mutex, safe client snapshot handoff/interpolation around100ms. GLFW line-art game with triangular colored ships, asteroid outlines, bullets/particles and HUD connection/player/lives/score/ping/wave. Native security remains mandatory. Preserve independent baseline IDL/build and all existing security safeguards.

P3 acceptance: warning-free make and make test, actual deterministic sim boundary/collision/split/wrap/damage/respawn/restart tests; secure two-to-five-bot run demonstrates moving ships/fire/hits/score/damage/respawn/wrap and cap; gameplay summary counters reflect actual sim transitions. Dedicated controlled test scenarios may cover rare gameover/restart without changing normal gameplay. Two actual GUI clients run on Xvfb for>=10sec, screenshots viewed to confirm geometry/HUD and meaningful input replicated. Reviewer audits real implementation/snapshot validation/thread ownership and zero open newHIGH/CRITICAL. Tester records exact output and distinguishes controlled tests from spontaneous gameplay. P2 flood-speed and encrypted snapshot evidence carried into this phase. Max three loops, no unverified completion claims.

Orchestrator owns progress/AI log/README and user-facing run support; builder owns game source/IDL/unit sim/build; reviewer owns auditfix/review log; tester owns independent integration/GUI/regression harnesses/report. Coordinate shared file edits. No sudo, installs, commits/pushes, baseline/third_party edits or private key contents. Existing Linux credentials may require regeneration on reboot; verify presence/modes only before use.

### P3 loop1 runtime findings (2026-11-04)
Builder reports coherent make -j2 all test exit0, warning-free. Tester independently ran35s secure five-hunter-bot gameplay: moves9831 shots831 destroyed223 splits100 hits7 respawns6 wraps30 snapshots704 points16280 ticks2134, joins5 leaves5 timeouts0. Each client logged34 authoritative snapshots with changing positions; observed scores2520–3710 and minimum lives1–2. Actual all-player gameover/restart did not occur in this live run (zero), so retain independent collision-driven deterministic fixtures as separate evidence. GUI10.5s and full per-case security regression still underway. Reviewer found medium HUD freshness/rejection-visibility issues; builder addressing. No P3 final pass yet.

Reviewer independently closed native no-cert observability implementation gap using a real secure/insecure participant probe: chained DDS_UNAUTHENTICATED_DISCOVERY/nonsecure-spdp event identity0. This is genuine native discovery of an uncredentialed peer under denying governance, not a fabricated certificate-check failure. Strict no-admission/event-delta full-game test pending.

## P2 resumed acceptance: PASS for local native-security controls
Tester final strengthened security harness exited0 after correcting the test's own Leave-flood recovery assumption (an abusive identity's last Leave can be rate-limited; correct3s lease then frees slot). Per-negative native no-cert discovery audit/noJoin, wrongCA audit, local forbidden-writer denial/noRemoteJoin, slow-reader60 responses with survival/rejoin, input/join/Leave floods eachnewchainedRATE with honest tickprogress and actual receivedspeed bounds all passed. Actual one-bit344-byte ciphertext send caused real OpenSSLGCM failure and a new production failed-decode audit event. Audit136 records verified against trusted root; modification/truncation rejected. No fakeauthfailure used for uncredentialed discovery or locally denied publication. External tcpdump remains denied/unrun, human command retained as allowed bygoal.

## P3 loop2 status
Reviewer reports zero open HIGH/CRITICAL; new malicious snapshot fixtures pass. Builder corrected connected-only-with-fresh-ownshipHUD and keeps rejected GUI visible until dismiss. Tester reran latest twoGUI clients>=10.5sec and inspected actualscreenshots PASS. Initial five-bot35s livephysics/score/hits/respawn/wrap passed, and isolatedcollision-driven unitgameover/restart passed. Adding explicit off-by-default controlled server collision scenario to obtain actual network server-summary gameover/restart evidence; no directcounterfabrication. Native normal game remains unchanged. Tester final secure lifecycle with per-fault audit assertions also underway. P3 final acceptance waits controlledtransition and review.

## P3 acceptance: PASS (2026-11-04)
Actual warning-free build/unit fixtures, secure35s fivebot physics/score/damage/respawn/wrap run, final strengthened security and secure lifecycle tests, and latest twoGUI10.5s with real input/screenshot inspection pass. Reviewer zero open HIGH/CRITICAL. HUD freshness/rejection issues fixed and rerun. Controlledserver-only collision scenario offdefault now proves real networkgameover/restart: setup181, simgameover182, ordinaryhunterRrestart192; bothclients received gameover1tick184 then0tick194. Serverhits2/gameovers1/restarts1,shots93destroyed29wraps3. Not presented as spontaneous normalplay. Purefixtures additionally cover exact timers/split/poolboundaries.

Game implemented: secureco-op5slots,60Hzserver,20Hz encryptedbounded snapshots,100ms wrappedinterpolation, triangles/neonasteroids/bullets/particles, physics/fire/split/waves/score/lives/respawn/invuln/gameover/restart, HUD/connection/RTT. Local two-clientlauncher testedexit0, human playtest checklist delivered in docs/PLAYTEST.md. Root independently inspected actualGUI screenshots and saved one user preview outside repo; no generated illustration substituted.

## P4 plan and acceptance (in progress)
Builder fixes fail-open hardening verification and any actual threading/lifetime defects, builds release/ASan+UBSan/TSan without altering baseline/third_party. Reviewer independently checks ELF protections/testfailure behavior, shared-state/loan/cleanup assumptions and sanitizer evidence. Tester runs five-client join/leave/rejoin soak >=2min under available sanitizers, Valgrind invalidaccess/definiteleak checks, preserves raw outputs. Longer10min course-spec soak remains explicit if not run. Root verifies fresh source-copy build (not literalcleanclone because no commits allowed), README/launcher and userhandoff.

Acceptance: meaningful fail-closed PIE/NX/RELRO/BIND_NOW/canarycheck, make testpass, actual ASan/UBSan andTSan sessions clean, no Valgrind definiteleak/invalidaccess. Distinguish sanitizer runtime/environment startup failures from source defects using minimal reproducer; no claim pass if tools cannot run. No sudo/globalASLR/sysctl changes. At most3loops for codefixes; if actualenvironment prerequisite blocks a check, record exactreproduction/humanaction and keep gamecompletion distinct from fullverification. P5/P6 not yet done.

## Final checkpoint2026-11-04 — game implemented; P4 dependency gate

P0–P3 PASS with actual game, security, lifecycle, GUI and controlled real-network game-over/restart evidence. Local launcher works; README now starts with the verified Windows-to-WSL command and controls. Preview saved for the user; source remains in Desktop/Networking_CSD3116. No generated image substitutes for game capture.

P4 hardening verifier negative tests and final release/ASan/TSan ELF checks pass. Independent120s ASan/UBSan five-client churn:21joins,18leaves,3timeouts,peak5;16replacements, no diagnostic. Root re-inspected23 retained logs including audit and verified anchor (124.107s auditspan). Final audit-throttle guard got its own native ASan/UBSan callback regression, exits0 genuineevent, no diagnostics. Final fresh Linux public source-copy all/test/hardening exits0 warning-free; not a literal committed clean clone.

P4 remains INCOMPLETE: per-process setarch solves TSan startup, but actual network run exits66 with unsuppressed observed conflicts inside system uninstrumentedDDS (recvmsg/memcpy, mutex-destroy/unlock). Valgrind server+fiveclients free all heap, servererrors0, eachclientreports61–63 errors in one uninitialised-sendmsg context with nativeDDS/OpenSSLorigin. Neither is dismissed or suppressed. Overall zero-open-HIGH certification cannot be claimed. Exact diagnostics/artifacts/reproduction in TEST_REPORT and REVIEW_LOG.

Actual environmental gate: Ubuntu cmake missing, needed for an isolated matching instrumented DDS dependency build. CODEX_GOAL.md says stop on missing package/tool and provide human apt command. Human in Ubuntu: sudo apt update; sudo apt install -y cmake. No fourth-party installation or globalASLR/sysctl change by agent. All three subagents hit usage limits again; root used declared sequential fallback for final checks.

P5/P6 not advanced past the genuine P4 gate. Remaining course work: dependency-warning investigation; longer10-minute soak; literal clean clone after human commit; measured optimisation/benchmarks; slide/video drafts/package; human feel/LAN/Wireshark. Game implementation itself is complete and local verified launchable. No git commits/pushes, private key output or baseline/third_party changes.

Defence notes: fixed-tick authoritative physics prevents input-rate speedups; bounded snapshots are validated/copied before DDS loan return; exact native permissions bind identities and server authority; audit callbacks rate-limit with guarded atomic timestamps and trusted final roots; passing gameplay/hardening/ASan does not establish a clean TSan/Valgrind result.

## 2026-11-04 — user paused LAN work and requested a local checkpoint commit
User asked to leave two-computer testing for now, commit current work and list remaining tasks. This explicitly authorizes the local git commit for this checkpoint despite the default AGENTS.md human-commit restriction; no push or git-config change is authorized. LAN investigation applied no host/firewall/WSL changes and produced no claim of a completed cross-machine test. Current DDS configuration remains loopback-only.

Prioritised remaining work is docs/TODO.md: unresolved TSan/Valgrind dependency findings/CMake prerequisite; deferred reachable-network/WSL/cert setup and actual two-PC test; full10-minute soak, clean-clone/extracted-build, external packet/human checks; measured optimisation; slides/video/peer/AI/package. Gameplay/local tests and known verification limits are preserved. Source/documentation only will be staged after secret/artifact checks; build outputs/private credentials remain excluded. Commit result will be reported to the user after success.


## Current blocker: real secure GUI acceptance failed

Actual `xvfb-run ... tests/p3_gui.py --security-dir <persistent Linux bundle> --output-dir build/evidence/integration-gui-run1` exited1. Both windows rendered, but the server summary had joins2, peak1, timeouts2, inputs5, moves0, shots0, snapshots40, ticks1500. The first lease expired before the second player joined. Both final PNGs show WAITING FOR GAME STATE rather than a current connected two-player world. Root inspected both actual images. This contradicts accepting the merged game as working locally on this host, regardless of contributor historical reports. It is not cleared by the headless run3 pass.

Evidence: build/evidence/integration-gui-output.txt, integration-gui-exit.txt, integration-gui-run1/*.log and client1-10s.png/client2-10s.png. Cleanup completed; no successful manual GUI session is currently left open. Do not fabricate a manual-phase pass or open the game as if all own checks passed.

The current bounded integration attempt cycle is stopped at the original maximum-three-attempt rule. The middle missing-/tmp-bundle prerequisite failure is disclosed, not silently erased to extend the limit. Request a human decision to authorize a new targeted GUI/network starvation/hang investigation cycle. No fourth network attempt or second GUI attempt has been run. A diagnosis is not established: do not label native dependency, GPU, clock, or the harness as the cause without stronger evidence.

The merged implementation and all local work remain preserved. P1 integration acceptance is INCOMPLETE despite passing headless functionality. Native-security and P3 source exist; P2/P3 have not been newly accepted here. P4 OPEN HIGH TSan/Valgrind findings, full soak, external capture, two-machine verification, P5 measurements and P6 deliverables remain required by the original scope/PDF checklist.

## Authorized GUI repair cycle - current
User explicitly replied FIX: continue GUI investigation. New maximum-three-loop cycle authorized. Plan: reproduce current software-GL failure with per-process CPU/thread/RSS and wait-state samples; distinguish rendering resource contention from application/DDS blocking before edits. Compare one controlled rendering change only when installed support is verified. Keep native security enabled, three-second leases, two actual clients, unchanged movement/fire/state assertions, retained failed outputs. Fresh independent builder/reviewer/tester roles requested; report availability honestly. Acceptance remains two concurrent connected windows >=10seconds, real input movement/fire, zero unintended lease expiry, correct cleanup, and inspected screenshots. Then open WSLg session, print manual checklist and STOP until APPROVED P1 or FIX. No blanket P4/sanitizer/security completion claim.

### GUI cycle diagnosis and approved repair design
Fresh default software-GL reproduction failed; reducing LP_NUM_THREADS to2 lowered each client from74 to14threads but still caused2lease expiries. The old GUI harness printed PASS despite timeouts; current stricter checks reject that false positive and require distinct IDs, sustained own-state receipt, final fresh progress, zero timeouts/invalid sessions and two normal leaves. Diagnostic-only wrappers preserve arguments/results/errno and report real dds_waitset_wait requests50,000,000ns taking~4.47-4.78seconds. Read-only observer samples continued at~0.5s, lowCPU and amplememory; CPU starvation and clock jumps are not established. Native emptyguard probe12pairs and extended60pairs returned normally at~50ms with realtime/monotonic/DDS clocks aligned. Exact0.10.4 source shows mutex acquisition/reacquisition outside the APIcondition deadline; underlying internal cause is not yet proven.

Reviewer approved event-driven dataavailable listeners plus fixed ready-bitmask and CLOCK_MONOTONIC condition as the bounded application fix. No native positive timedwait/polling-sleep workaround; no new threads, IDL, QoS, crypto or lease changes. Callbacks only signal the queue, all DDS work is outside its lock, full batches requeue, startup contexts are stable, synchronous listener detach precedes cleanup. Builder implementing; tester will run fresh native headless lifecycle and strict original-default GUI tests without LPcap/preload. Known P4/native timer and lease-recovery limitations remain explicit; this is not a sanitizer or full-security pass.
