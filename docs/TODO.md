# Paused checkpoint - latest authoritative status

Stanley explicitly paused the goal on4November2026 and requested a teammate handover plus publication. Read HANDOVER.md first. The listener repair preserves active two-client GUI state in the latest targeted run, but overall GUI/network checks still FAIL native one-second Leave ACK timeout. No further game tests/repairs after pause. Selected portable raw evidence is in docs/evidence/2026-11-04-handover/. Earlier status text below is historical. No goal-complete or submission-ready claim.

---
# Latest integration status: GUI verification blocked

See the current blocker section below. Existing build/unit/headless passes are narrower than full graphical multiplayer acceptance.

# Current integration priorities

See INTEGRATION.md for the reconciliation with our PDF checklist and local approval gates.

- [ ] Investigate the fresh P1 run1 server/network shutdown hang; run3 passed but root cause remains unproven.
- [ ] Finish current secure launcher/manual P1 acceptance; preserve --baseline mode and all prior evidence.
- [ ] Continue the original P2-P6 verification/materials below after the relevant approvals. Existing implementation is reused, not rebuilt.
- [x] Record actual team roster: Stanley Lur (leader), Jordain Ng; no team name. Responsibilities still supplied by the team.
- [x] Confirm supplied Team Lab PDF deadline: Friday6November2026. Moodle submission itself remains human/unverified.

The following contributor checklist is retained, including historical claims. Its two-computer pause note is context from that contribution, not a new instruction in this chat.

---
# Remaining work — DDS Asteroids

Checkpoint: 2026-11-04. Two-computer setup is paused at the user's request. Gameplay and local networking are implemented; full assignment verification is incomplete.

## 1. Resolve memory/thread verification findings (highest priority)

- [ ] Human installs missing CMake inside Ubuntu: `sudo apt update` then `sudo apt install -y cmake`.
- [ ] Build/investigate a matching instrumented Cyclone DDS/security dependency in isolated Linux storage; keep release protections and the original reference files intact.
- [ ] Resolve unsuppressed TSan reports in the current DDS recvmsg/memcpy and mutex lifetime paths. Do not declare them false positives merely because the system library is uninstrumented.
- [ ] Resolve Valgrind's uninitialised bytes reaching client sendmsg through native DDS/OpenSSL crypto. All six processes freed their heap, but the reported errors still fail acceptance.
- [ ] Rerun relevant security, sanitizer and Valgrind sessions after actual fixes, retaining raw outputs. Overall zero-open-HIGH status has not been achieved.

Evidence: TEST_REPORT.md P4 checkpoint; REVIEW_LOG.md final dependency findings. ASan/UBSan two-minute churn and ELF hardening passed; this does not clear the unresolved tools.

## 2. Two-computer/VPN network test (deferred)

- [ ] Confirm both PCs' reachable network addresses and whether they share a LAN or use a VPN/routed connection. Same LAN is not an explicit project requirement; reachability is required.
- [ ] Configure a LAN/VPN DDS interface and explicit peer addresses. Current config/cyclonedds.xml remains loopback-only; no LAN helper/config was committed as working.
- [ ] Address WSL2 NAT/mirrored networking and narrowly scoped Windows/Hyper-V UDP firewall settings. Do not globally disable firewalls or shut down unrelated WSL work without coordinating.
- [ ] Provision minimal separate server/client bundles from one trusted CA/policy set. Never distribute the complete CA fixture or put private keys in git/zip/logs.
- [ ] Run one server and a client on each computer with distinct identities. Verify movement, firing, shared score/lives, leave/rejoin and killed-client slot recovery.
- [ ] Record actual latency/bandwidth and packet-loss/delay behavior; implement/test an appropriate controlled impairment method if required by the final brief.
- [ ] Retain actual cross-machine results; same-PC windows and socket observations are not a substitute for this test.

The second PC reportedly has Windows/Ubuntu/WSL and the project. Shared-network status remains unknown. Investigation only read local settings; no host networking or firewall changes were applied.

## 3. Finish verification and reproducibility

- [ ] Run the full ten-minute five-client sanitizer soak with random join/leave/rejoin. Completed churn so far is 120seconds, not ten minutes.
- [ ] After committing, verify build/run from an actual clean clone. A fresh public source-copy build passed; no literal clone run has been completed.
- [ ] Perform external authorised tcpdump/Wireshark inspection of protected gameplay traffic. In-process postcrypto GCM observations passed, while unprivileged external capture was denied.
- [ ] Complete human control/smoothness/visual and two-machine playtest in PLAYTEST.md.
- [ ] Review documented limits: 64 sessions per identity per server lifetime, automatic session recovery after server restart absent, HUD can overlap asteroid graphics, Ubuntu x86_64 plugin path dependency.

## 4. Optimisation evidence (P5)

- [ ] Prepare headless benchmarks and measured before/after tables in PERFORMANCE.md.
- [ ] Cover required compiler, branch, cache/layout, dependency-chain, SIMD and algorithmic improvements with defensible measurements rather than unverified claims.
- [ ] Compare idle CPU and relevant latency/bandwidth. Fixed pools and cached glyphs exist but are not a complete profiler report.
- [ ] Run short course Linux server perf measurements if available; otherwise clearly label local clock_gettime/Cachegrind evidence and prepare exact server commands.

## 5. Submission and team materials (P6)

- [ ] Confirm the original lecturer brief/deadline. Repository prompt records Friday 6 November 2026; it has not been checked against Moodle.
- [ ] Fill actual team names, roles/contributions and code-walkthrough defence notes; no human contribution should be invented.
- [ ] Create slides/PDF: design diagrams, gameplay, networking, optimisation tables, threat model, negative-test evidence and AI declaration.
- [ ] Draft and record a team-narrated MP4 of at most 5 minutes / 200 MB; all members narrate, no AI voice per saved prompt.
- [ ] Complete peer evaluation and AI-use declarations in source, slides and video.
- [ ] Produce/review source ZIP excluding keys, private provisioning fixtures, generated binaries/runtime captures and unwanted artifacts; verify extracted source builds.
- [ ] Reconcile final checklist with actual test evidence and disclose remaining issues rather than marking incomplete items done.

## Already implemented / locally demonstrated

- [x] C/OpenGL/GLFW/Cyclone DDS co-op Asteroids, server-authoritative 60 Hz simulation, 2–5 players.
- [x] Ships/physics/fire, splitting asteroids/waves, score/lives/respawn/invulnerability, wrap, game-over and restart.
- [x] Native certificate authentication, signed topic roles, encrypted snapshots, spoof/replay/flood checks and chained audit evidence.
- [x] Five-player local gameplay, controlled real-network game-over/restart, two GUI windows with actual inputs.
- [x] Meaningful protocol/sim tests, hardening positive/negative tests, 120-second ASan/UBSan churn, fresh-source-copy build.
- [x] Local two-window launcher and current README/test/review/AI documentation.

Start with item 1 when verification resumes; two-computer work remains paused unless the user resumes it. No new tests or implementation were implied merely by this checklist.


## Current blocker: real secure GUI acceptance failed

Actual `xvfb-run ... tests/p3_gui.py --security-dir <persistent Linux bundle> --output-dir build/evidence/integration-gui-run1` exited1. Both windows rendered, but the server summary had joins2, peak1, timeouts2, inputs5, moves0, shots0, snapshots40, ticks1500. The first lease expired before the second player joined. Both final PNGs show WAITING FOR GAME STATE rather than a current connected two-player world. Root inspected both actual images. This contradicts accepting the merged game as working locally on this host, regardless of contributor historical reports. It is not cleared by the headless run3 pass.

Evidence: build/evidence/integration-gui-output.txt, integration-gui-exit.txt, integration-gui-run1/*.log and client1-10s.png/client2-10s.png. Cleanup completed; no successful manual GUI session is currently left open. Do not fabricate a manual-phase pass or open the game as if all own checks passed.

The current bounded integration attempt cycle is stopped at the original maximum-three-attempt rule. The middle missing-/tmp-bundle prerequisite failure is disclosed, not silently erased to extend the limit. Request a human decision to authorize a new targeted GUI/network starvation/hang investigation cycle. No fourth network attempt or second GUI attempt has been run. A diagnosis is not established: do not label native dependency, GPU, clock, or the harness as the cause without stronger evidence.

The merged implementation and all local work remain preserved. P1 integration acceptance is INCOMPLETE despite passing headless functionality. Native-security and P3 source exist; P2/P3 have not been newly accepted here. P4 OPEN HIGH TSan/Valgrind findings, full soak, external capture, two-machine verification, P5 measurements and P6 deliverables remain required by the original scope/PDF checklist.
