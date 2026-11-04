# Lecturer requirement check - 5 November 2026

Purpose: answer the user's request to test the automatic launcher and verify the supplied WSL/OpenGL/GLFW/DDS, multiplayer, distributed/security and class-concept requirements. Single-agent review and targeted tests only; no application repair, full-suite rerun, sanitizer rerun, benchmark or privileged capture in this check.

## Approval decision

**Automatic launcher and tested local multiplayer behavior: PASS. Full rubric/complete distributed-security approval: NOT GRANTED.** Cross-machine operation and actual external encryption capture remain unverified. Cached computation and blocking waits demonstrate applied concepts, not a measured optimization result. Known unsuppressed dependency findings and the approved Leave ACK limitation remain open.

| User-supplied requirement | Evidence | Decision / limit |
|---|---|---|
| WSL (macOS allowed alternative) | Actual kernel6.18.54.1-microsoft-standard-WSL2 | PASS WSL route; macOS is an alternative, not an additional test requirement |
| OpenGL, GLFW, Cyclone DDS | Executed client linked libGL.so.1, libglfw.so.3, libddsc.so.0debian; actual rendered windows and DDS state exchange | PASS required stack on this machine |
| Minimum two players | Actual auto-launcher admitted two distinct IDs/windows and maintained fresh state for more than10seconds; prior clone test ran124.539seconds | PASS local two-player operation |
| Real-time and interactive | Real keyboard thrust/rotation/fire; moving ships,14shots, ongoing snapshots and shared HUD; both new final screenshots inspected | PASS tested local behavior; not a human feel judgment or cross-machine latency measurement |
| Distributed | Separate server/client processes exchange authoritative state via DDS; config remains domain42/loopback | Architecture implemented; two-computer/LAN/VPN operation NOT VERIFIED |
| Secure: client authentication | Corrected targeted wrong-CA and no-cert tests each rejected with exit1, no extra slot, normal client remained alive, audit chain verified | PASS tested certificate rejection cases; not every threat-model test rerun |
| Secure: DDS encryption | Generated governance requires encrypted RTPS/metadata/data and denies unauthenticated participants; historic GCM tamper rejection in EVIDENCE.md | Implemented/configured; external packet proof still PENDING. HUD text alone is not proof |
| Improve baseline: client leaving | Auto-launch first client exits gracefully; server records Leave; launcher stops session exit0; joins2/leaves2/timeouts0/invalid0. Earlier T1 lifecycle proves rejoin/cap/crash lease | PASS tested leave and lifecycle improvements; ACK-timeout fallback is an explicit limitation |
| Class concepts: threading and avoid repeated computation | src/common/event_queue.c uses bounded condition waits; src/client/render.c:19-24 regenerates glyph vertices only when text changes | Applied patterns confirmed; no numerical idle-CPU/speedup claim or race-free certification |
| Class concepts: security | Native certificate authentication, signed governance/permissions, protected role topics, historical negative tests | Applied; capture and existing dependency findings remain limits |

## Exact tested launcher

```bash
env -u LD_PRELOAD -u LP_NUM_THREADS -u CYCLONEDDS_URI LIBGL_ALWAYS_SOFTWARE=1 \
  xvfb-run -a -s '-screen 0 1620x650x24' python3 -B \
  build/evidence/requirements-launcher-20261105-01.py
```

The preserved harness invokes the real `bash tools/play_local.sh --duration 45` with its automatic credential selection, rather than bypassing the launcher with manual server/client commands. It admits both clients, finds windows by exact PID/title, sends real key events, checks own-state freshness and sustained progress, captures new screenshots, signals the exact first-client pidfd, verifies Leave/session exit, and cleans its own process group only on failure. Software GL/Xvfb is automation evidence, not proof of visible WSLg desktop windows. The normal human command remains `bash tools/play_local.sh` in Ubuntu; closing one client ends that managed session. Use RUN_AND_VERIFY.md's separate terminals for manual leave/rejoin.

### Actual launcher output

```text
SCREENSHOT /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/evidence/requirements-launcher-20261105-01/client1-launcher.png
SCREENSHOT /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/evidence/requirements-launcher-20261105-01/client2-launcher.png
FRESH_STATE client=1 observed_age_seconds=1.667
FRESH_STATE client=2 observed_age_seconds=0.911
LIVE_STATE client=1 id=1 samples=15 first_tick=43 last_tick=901
LIVE_STATE client=2 id=2 samples=14 first_tick=83 last_tick=879
PASS automatic launcher: server first, two admitted distinct GUI clients, real key input, fresh state >10s
SERVER_SUMMARY joins=2 rejects=0 leaves=2 timeouts=0 invalid=0 stale=0 peak=2 inputs=288 rate_hits=0 auth_failures=0 middleware_events=0 moves=1627 shots=14 destroyed=3 splits=3 hits=0 respawns=0 gameovers=0 restarts=0 wraps=0 snapshots=310 points=90 ticks=944
PASS client Leave received; automatic session shutdown exit0
```

[Player1 final image](evidence/2026-11-05-requirements/client1-launcher.png), [player2 final image](evidence/2026-11-05-requirements/client2-launcher.png). Both show CONNECTED / DDS SECURE, distinct ships, asteroids and matching scores20/70 and lives3/3. They were visually inspected once. No screenshot was reopened.

### Actual corrected authentication output

```text
PASS wrong-ca: no admission, client exit=1, legitimate peer remains alive
PASS no-cert: no admission, client exit=1, legitimate peer remains alive
SERVER_SUMMARY joins=1 rejects=0 leaves=1 timeouts=0 invalid=0 stale=0 peak=1 inputs=59 rate_hits=0 auth_failures=6 middleware_events=7 moves=0 shots=0 destroyed=0 splits=0 hits=1 respawns=1 gameovers=0 restarts=0 wraps=0 snapshots=132 points=0 ticks=412
PASS rejected peers used no slot; authentication and unauthenticated-discovery audit evidence verified
```

**Failed attempt retained:** first targeted run selected wrong-CA identity2, but tools/gen_certs.sh generates the rogue certificate only for identity6. The client failed locally with missing identities/rogue_client2.crt, so no server authentication-warning audit event existed; harness exit1 was correct. This was a test-setup error, not evidence that an untrusted certificate reached/rejected by the server. One correction selected the actual rogue identity6 and reran into a fresh output directory; the output above passed and server auth_failures6 plus audit evidence were verified. No product code, security policy or assertion was weakened. [Original failed output](evidence/2026-11-05-requirements/requirements-auth-20261105-01-output.txt).

### Actual WSL/linkage/cleanup output

```text
WSL kernel: 6.18.54.1-microsoft-standard-WSL2
PASS linked libddsc.so.0debian
PASS linked libglfw.so.3
PASS linked libGL.so.1
PASS no remaining game/Xvfb processes
```

## What prevents complete approval

1. Actual protected gameplay pcap has not been supplied/analyzed. The capture helper passed syntax and synthetic full-packet filter checks only. User runs sudo capture; agent analyzes without sudo. The prior empty capture is no evidence.
2. Current config deliberately selects loopback. No two-computer test, secure remote provisioning or LAN/VPN performance evidence is available; excluded from the slim scope unless the user changes it.
3. Optimization concepts exist but controlled idle before/after and baseline/optimized perf measurements are missing. Prepared course commands are not measurements.
4. Native Leave transport ACK can time out; approved timeout-only warning/exit0 does not prove delivery. Fresh server release and replacement admission are separate assertions. Existing TSan/Valgrind dependency findings remain unsuppressed and outside this test scope.

No new game/runtime failure was observed in the automatic launcher test or corrected authentication run. This does not override the unresolved items above or imply a grade guarantee. Complete raw outputs, failed attempt and reproducible harnesses are stored under docs/evidence/2026-11-05-requirements/. Full original logs remain in ignored build/evidence/. All test processes were cleaned up.
