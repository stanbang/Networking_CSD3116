# Evidence for slides - 5 November 2026

This is an evidence inventory, not slides or a claim that the assignment rubric is fully satisfied. Slim finish mode excludes further GUI investigation, TSan/Valgrind dependency investigation, two-computer testing, slide production and video production. No new benchmark or sanitizer run was performed. T3 was approved by the user. T4 live capture/analysis remains pending. README, handover, AI-use notes and source-ZIP instructions are updated in RUN_AND_VERIFY.md; no final archive/submission is claimed.

## Reproduction provenance

Fresh GitHub clone base: `3dae8e3b236848b5a2b4fc584b1ba83f14395ea4`. Current uncommitted working changes were applied as a recorded patch, and two explicit new test fixtures were copied. This is **a fresh clone plus working patch**, not a published, clean committed revision. Verification directory: `/home/stanley/.local/state/dds-asteroids/slim-check-20261105-01/source`; patch retained beside it. The clone includes no credential bundle, key/PEM/capture files from the source inventory; runtime uses an existing private Linux-home fixture outside the clone. No private keys are included in these evidence files.

| Check | Actual evidence and scope |
|---|---|
| Fresh build | `make -j2`, exit0; [full compiler output](evidence/2026-11-05-slim-finish/slim-t2-build.txt) |
| Units | `make test`, exit0; [full output](evidence/2026-11-05-slim-finish/slim-t2-units.txt) |
| Hardening | `make check-hardening`, exit0; both binaries PIE, NX, RELRO, BIND_NOW and canary references; [full output](evidence/2026-11-05-slim-finish/slim-t2-hardening.txt) |
| Headless | Two clients, 124.539 seconds after both admitted; no unexpected timeout, fresh own-state streams, explicit leaves, audit verified; [output](evidence/2026-11-05-slim-finish/slim-t2-headless-output.txt) |
| Strict GUI | One fresh Xvfb/software-GL run, no preload or thread cap, exit0; [output](evidence/2026-11-05-slim-finish/slim-t2-gui-output.txt). New final images visually inspected once: both CONNECTED / DDS SECURE, two ships, asteroids, matching score/lives HUD. This is same-machine automation, not human smoothness or distributed validation. |
| Approved Leave fallback | Real targeted lifecycle exit0, fresh per-session slot-release events and final replacement; explicit timeout injection separately exercised warning/exit0 and real server Leave audit. [Lifecycle](evidence/2026-11-05-slim-finish/slim-t1-network1-output.txt), [injected timeout](evidence/2026-11-05-slim-finish/slim-t1-forced-timeout-output.txt) |

### Actual hardening output

```text
bash tools/check_hardening.sh build/release/server build/release/client
== build/release/server
  Type:                              DYN (Position-Independent Executable file)
  GNU_STACK      0x000000 0x0000000000000000 0x0000000000000000 0x000000 0x000000 RW  0x10
  GNU_RELRO      0x00a7f0 0x000000000000a7f0 0x000000000000a7f0 0x000810 0x000810 R   0x1
 0x000000000000001e (FLAGS)              BIND_NOW
 0x000000006ffffffb (FLAGS_1)            Flags: NOW PIE
PASS PIE NX RELRO BIND_NOW CANARY references=2
== build/release/client
  Type:                              DYN (Position-Independent Executable file)
  GNU_STACK      0x000000 0x0000000000000000 0x0000000000000000 0x000000 0x000000 RW  0x10
  GNU_RELRO      0x00d6b0 0x000000000000e6b0 0x000000000000e6b0 0x000950 0x000950 R   0x1
 0x000000000000001e (FLAGS)              BIND_NOW
 0x000000006ffffffb (FLAGS_1)            Flags: NOW PIE
PASS PIE NX RELRO BIND_NOW CANARY references=2
```

### Actual two-client headless output

```text
PASS two-client headless elapsed_after_both_admitted_seconds=124.539
SERVER_SUMMARY joins=2 rejects=0 leaves=2 timeouts=0 invalid=0 stale=0 peak=2 inputs=2366 rate_hits=0 auth_failures=0 middleware_events=0 moves=14867 shots=1240 destroyed=272 splits=117 hits=1 respawns=1 gameovers=0 restarts=0 wraps=0 snapshots=2393 points=20180 ticks=7542
PASS distinct identities, sustained own-state streams, explicit leaves and audit chain
```

### Actual strict GUI output

```text
FRESH_STATE client=1 observed_age_seconds=1.279
FRESH_STATE client=2 observed_age_seconds=0.673
LIVE_STATE client=1 id=1 samples=12 first_tick=69 last_tick=743
LIVE_STATE client=2 id=2 samples=11 first_tick=105 last_tick=719
SERVER_SUMMARY joins=2 rejects=0 leaves=2 timeouts=0 invalid=0 stale=0 peak=2 inputs=222 rate_hits=0 auth_failures=0 middleware_events=4 moves=747 shots=12 destroyed=3 splits=3 hits=2 respawns=2 gameovers=0 restarts=0 wraps=0 snapshots=253 points=90 ticks=772
PASS secure two-GUI lifecycle, real key input and >=10s rendering; inspect screenshots
```

Reproduction commands from the verification clone (credentials remain outside source):

```bash
make -j2
make test
make check-hardening
# Headless orchestration script and full result retained in the original checkout:
# build/evidence/slim-t2-headless-script.py and slim-t2-headless-output.txt
# It starts server first, admits identities1/2 sequentially, runs clients125s,
# checks >=120s after both admissions, fresh own-state streams and exact audit/summary.
env -u LD_PRELOAD -u LP_NUM_THREADS -u CYCLONEDDS_URI LIBGL_ALWAYS_SOFTWARE=1   xvfb-run -a -s '-screen 0 1620x650x24' python3 -B tests/p3_gui.py   --security-dir "$BUNDLE" --output-dir "$NEW_GUI_OUTPUT"
```

`BUNDLE` must name the user's private Linux-home fixture; `NEW_GUI_OUTPUT` must not already exist. An injected public-API timeout is application fault-injection evidence, not proof of successful native transport acknowledgement.

## Security negative tests: historical evidence versus current checks

The following is copied from existing `TEST_REPORT.md`, section "P2 sequential fallback verification - checkpoint, not phase completion" (4 November). It was not rerun as a new P2 suite in slim mode. Earlier temporary credentials/log locations in that report may no longer exist. The current two-client build does not independently revalidate every historical negative test.

```text
PASS no-cert: no accepted join, exit=1; see native diagnostics in build/p2-test/no-cert.log
PASS wrong-ca: no accepted join, exit=1; see native diagnostics in build/p2-test/wrong-ca.log
PASS rogue-server: no accepted join, exit=1; see native diagnostics in build/p2-test/rogue-server.log
PASS slow reader: 60 responses; legitimate player and server survive
PASS fresh authenticated join after slow-reader departure
PASS SERVER_RATE identity=6 kind=input
PASS SERVER_RATE identity=6 kind=join
PASS actual ciphertext send: byte=52 packet_bytes=336
PASS receiver OpenSSL GCM authentication failure observed
PASS audit chain records=117 root=f48ff0db315e9af259b3eef248891028b019ef9aed389c47979750342aab90ce
PASS audit modified detected: Record 1: content hash mismatch
PASS audit truncated detected: Final root differs from trusted server root
PASS P2 harness assertions; native rejection/rate details require report evidence
```

Current T1 native lifecycle additionally verifies spoof/session/id-zero/id-high/replay drops, five-player capacity, explicit release/replacement and intentional crash lease expiry. Its actual output is linked above. Historical ciphertext mutation was an in-process post-crypto injection with observed GCM failure. It is **not** an external tcpdump confidentiality proof. T4 external packet evidence remains missing; the earlier human capture contained zero packets. The updated helper passed syntax/invalid-argument/offline RTPS filter checks with a complete2042-byte synthetic packet; this does not establish live capture or encryption. See [capture-helper checks](evidence/2026-11-05-slim-finish/slim-t4-capture-check.txt).

## Idle CPU before/after

| Metric | Before | After | Defensible conclusion |
|---|---|---|---|
| Controlled idle server CPU% | Not measured | Not measured | No numerical improvement claim |
| Controlled idle client CPU% | Not measured | Not measured | No numerical improvement claim |

Existing scheduling diagnostics were collected under different active GUI/churn conditions. Thread counts and CPU samples cannot be relabelled as a controlled idle before/after benchmark. The listener change improved the observed connected GUI state, but it does not supply a measured idle CPU improvement. No new benchmark was run under slim mode.

## Course Linux server perf commands (prepared, not run)

No benchmark executable is supplied by the existing Makefile, so benchmark measurements remain missing. Before using these commands, supply two actual headless, equal-workload benchmark binaries compiled with `-O2`: baseline and optimized. Keep each invocation about0.5seconds, validate equal results first, and use short shared-server jobs. Set the paths to your real binaries; these commands do not build or invent them.

```bash
BASELINE_BIN=/absolute/path/to/baseline-benchmark
OPTIMIZED_BIN=/absolute/path/to/optimized-benchmark
test -x "$BASELINE_BIN" && test -x "$OPTIMIZED_BIN" || exit 1
perf stat -r 20 -e cycles,instructions,branches,branch-misses,L1-dcache-loads,L1-dcache-load-misses -- "$BASELINE_BIN" 2> baseline-perf.txt
perf stat -r 20 -e cycles,instructions,branches,branch-misses,L1-dcache-loads,L1-dcache-load-misses -- "$OPTIMIZED_BIN" 2> optimized-perf.txt
```

These require permission/access on the course server; none was assumed. Report elapsed time together with instructions/cycles, IPC, branch-miss rate and L1 miss rate; local WSL clock/Cachegrind output is not hardware perf evidence. No profiler table with invented numbers is supplied.

## Known limitations and explicitly excluded work

- **Transport Leave ACK remains unresolved.** User approved timeout-only warning/exit0 after successful write on5November. A warning is not server receipt. Tests now require fresh server release evidence and replacement admission; other errors remain failures. Original native timeout evidence stays in `docs/evidence/2026-11-04-handover/ack-timing.txt`. The fallback is explicitly a changed acceptance policy, not a native DDS repair.
- **TSan/Valgrind dependency findings remain open and out of scope.** HANDOVER.md retains native recvmsg/memcpy race/mutex-lifetime and uninitialized crypto-sendmsg findings. They were not rerun, suppressed or declared false positives. This finish mode does not establish memory/thread-sanitizer cleanliness.
- **Two-computer testing remains unverified and out of scope.** Current DDS configuration is domain42 loopback. Local windows and the fresh clone do not establish LAN/VPN reachability, latency/bandwidth/loss or secure per-machine provisioning.
- **Further GUI investigation is out of scope.** One required strict GUI run passed; older waitset/lease/shutdown failures remain historical evidence. The prepared clock-witness diagnostic was not run. Previously observed backwards realtime reading is unexplained and must not be presented as the cause of ACK failure.
- **Slides and video production are out of scope.** This evidence document is not the presentation PDF or an actual team-narrated MP4. Real member contributions, narration, peer evaluation and human playtest remain team responsibilities. No all-rubric-met or grade guarantee is made.
- **Optimization evidence is incomplete.** Controlled idle CPU before/after and baseline/optimized benchmark binaries/results are absent; commands alone are not measurements.
- **Capture evidence is pending.** No successful external protected-gameplay capture has been analyzed. Packet metadata visibility and encrypted payloads must be evaluated from the user's actual capture in T4.
- **Published reproducibility is incomplete.** Current verification used a fresh clone plus uncommitted working patch, not a published final commit. Keys and private provisioning must stay outside the source ZIP.
- Existing application limits retained from HANDOVER/TODO: at most64 admitted sessions per identity per server lifetime, no automatic recovery after server restart/expired session, and Ubuntu x86_64 native plugin-path dependency. No claim that these were fixed in slim mode.


## Latest targeted launcher/authentication checks

See [REQUIREMENTS_CHECK.md](REQUIREMENTS_CHECK.md) for fresh automatic-launcher and corrected certificate-rejection evidence, the retained failed fixture attempt, and the explicit partial-approval decision. This does not turn the historical full P2 table into a fresh complete-suite pass.
