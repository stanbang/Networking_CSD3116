# Pulled-code review — 4 November 2026

Reviewed `3dae8e3` against `2cad2e1` on Jordain's Windows/Ubuntu WSL machine. Three real agents handled source review, build/testing and deliverable audit; root coordinated runtime checks and screenshot inspection. This review does not approve every phase or declare submission readiness.

## Teammate additions

- Bounded listener notifications and event-queue tests replace the problematic waitset path.
- Stricter network/GUI freshness, identity, lease and shutdown assertions.
- Audit overwrite protection, unique evidence folders, secure managed launch/stop tools.
- Handover evidence and assignment/PDF mapping. The existing game/security implementation is retained.

## Fresh checks on Jordain's computer

| Check | Actual result |
|---|---|
| `make -B -j2 all test check-hardening` | Exit 0; forced release rebuild, all three unit suites and both executable hardening checks pass; no compiler warnings |
| Native-secure `tests/p1_network.py` | Exit 0; five distinct admissions, sixth rejection, leave/rejoin, SIGKILL lease recovery, spoof/session/invalid-ID/replay rejection, audit chain verified |
| Native-secure `xvfb-run ... tests/p3_gui.py` | Exit 0; two distinct clients, fresh state, actual GLFW movement/fire, normal shutdown |
| Root visual inspection | Both final 800×600 screenshots show CONNECTED / DDS SECURE, P1/P2 identities, ships, asteroids and HUD |
| Cleanup | No server/client process remained |
| CMake prerequisite | Human installed it; root verified version 3.28.3 |
| Full P2 security negatives | Exit 0; no/wrong certificate, forbidden publisher, slow reader, floods, actual encrypted-byte corruption and audit mutation/truncation checks pass |
| Literal clean clone | Committed HEAD `3dae8e3` builds warning-free and passes units/hardening; pending local edits are excluded from this clone result |

Selected actual output:

```text
PASS no-event monotonic timeout elapsed_ns=50112353
SERVER_SUMMARY joins=12 rejects=1 leaves=11 timeouts=1 invalid=4 stale=1 peak=5 inputs=358 rate_hits=0 auth_failures=0 middleware_events=7 moves=2343 shots=199 destroyed=27 splits=12 hits=0 respawns=0 gameovers=0 restarts=0 wraps=0 snapshots=223 points=1980 ticks=671
PASS P1 networking lifecycle and malformed-message counters
SERVER_SUMMARY joins=2 rejects=0 leaves=2 timeouts=0 invalid=0 stale=0 peak=2 inputs=227 rate_hits=0 auth_failures=0 middleware_events=4 moves=1076 shots=11 destroyed=2 splits=2 hits=0 respawns=0 gameovers=0 restarts=0 wraps=0 snapshots=258 points=40 ticks=776
PASS secure two-GUI lifecycle, real key input and >=10s rendering; inspect screenshots
```

The single P1 timeout is intentional SIGKILL lease recovery. The earlier genuine native Leave ACK timeout and intermittent server shutdown hang on Stanley's machine did not reproduce here. **They remain unresolved cross-host findings; one passing run does not establish a fix.** No production source change was needed for today's passes.

Commands ran from `/mnt/c/Users/jorda/Desktop/Networking_CSD3116` against Cyclone DDS 0.10.4, with fresh credentials in private Linux storage. No marker preload or Mesa thread cap was used. Raw output/logs/PNGs are in ignored `build/evidence/current-review/`; selected public text output is under `docs/evidence/2026-11-04-jordain-review/`. The later P2 rerun passed; ten-minute ASan/UBSan and exact-source dependency verification are being recorded separately as they complete.

## Remaining work, in priority order

1. **Shutdown consistency:** investigate the preserved `leave_ack: Timeout` and server hang on the affected host; retain strict network/GUI results there after an actual fix. Preserve earlier failed evidence.
2. **Thread/memory safety:** CMake and exact Ubuntu-source symbol/TSan dependency builds are now ready, including all three security plugins. Resolve existing TSan DDS race/mutex-lifetime and Valgrind uninitialised crypto-send findings without blanket false-positive claims. Rerun relevant suites, including listener teardown and the full ten-minute five-client sanitizer churn.
3. **Security evidence:** full negative-security matrix passed against the pulled listener revision. Obtain and inspect authorised external protected-game packet capture. The updated helper now supports `--secure` full packets; its default remains truncated P0 capture. Helper/offline filter checks alone are not external capture evidence.
4. **Two-computer networking:** configure reachable LAN/VPN peers, WSL/firewalls and separate minimal identity bundles; verify gameplay, leave/rejoin and crash recovery. Record latency/bandwidth and controlled loss/delay results. Current DDS XML remains loopback-only; previously deferred two-computer setup was not performed here.
5. **Performance:** `bench/` has only `.gitkeep`; `docs/PERFORMANCE.md` is absent. Produce measured before/after compiler, branch, cache/layout, dependency-chain, SIMD, algorithm and idle-CPU evidence, with short course-server perf commands where available.
6. **Submission:** slides/PDF, diagrams, actual contributions, defence notes, AI declarations, peer evaluation, source ZIP and real team-narrated MP4 (at most five minutes/200 MB; no AI voice). Verify a literal clean clone and extracted ZIP build. Complete human playtesting. Final materials are not present as tracked deliverables.
7. **Documentation:** README quick-start paths and current status have been reconciled, with earlier host failures retained as open findings. Keep phase/PDF status rows and final materials tied to actual evidence.

Continuation changes: native Leave diagnostics (reader counts/result/timing; same mandatory one-second ACK), build-time isolated security-plugin path selection, evidence-safe P2/P4 harness output, secure full-packet capture mode and documentation. No commit/push, host-network/global setting changes or package installation by agents. The C/OpenGL/GLFW/Cyclone DDS stack is unchanged.
