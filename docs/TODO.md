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
