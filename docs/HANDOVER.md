# Latest teammate takeover - 5 November 2026

Stanley requested a teammate handover and publication of completed work. Read [TEAMMATE_HANDOVER_2026-11-05.md](../TEAMMATE_HANDOVER_2026-11-05.md) first. It lists verified work, open gates/tests, requested but unimplemented login/high-score/remember-user tasks for Jordain, contribution honesty and source-ZIP inclusions/exclusions. Existing blocked capture and other limitations remain. Historical status below is preserved.

# CURRENT: T3 approved; human game test and capture pending - 5 November 2026

User approved T3 with an additional request for exact build/run/manual verification instructions. See docs/RUN_AND_VERIFY.md. Capture helper now requests full IPv4 packets; syntax, invalid arguments and synthetic2042-byte filter checks passed, but live capture/analysis remain pending. No agent sudo. README points to the new guide. Prior approval/status text below is historical. T4 capture analysis and final packaging verification are not complete.

# CURRENT: T3 evidence ready, awaiting approval - 5 November 2026

T1 fallback approved and verified. T2 fresh-clone-plus-working-patch checks passed. T3 evidence and disclosed limitations are in docs/EVIDENCE.md. Stop until APPROVED T3 or FIX. T4/T5 not started. Older status headings below are preserved history.

# T1 approved fallback verified; T2 next - 5 November 2026

See the appended T1 verification below. Prior awaiting-approval text is historical.

# T1 fallback approved - 5 November 2026

User explicitly replied "Approved T1". Implemented timeout-only warning after successful Leave write; all other write/ACK/cleanup failures still fail. This is an accepted known limitation, not a transport ACK fix. P1 lifecycle tests now require a fresh per-identity server Leave/slot-release event after each graceful stop and retain replacement-admission checks, including a final replacement. Same-identity sessions are sequential, so a previous session's audit entry cannot satisfy the count increase. Production server clears the slot before emitting the event. Existing failure evidence is preserved. Review and targeted runtime verification pending; T2 not started.

---

# SLIM FINISH MODE - current scope, 5 November 2026

This replaces the earlier goal and its automatic phase workflow. Single agent; earlier delegated work stopped. Preserve existing implementation and evidence. No broad scans, dependency internals, fresh sanitizer investigations, or rebuilding the game from scratch. At most one repair attempt per issue; targeted tests only, full suite once at the end, two-minute soak. Stop for approval after T1 and T3. Never run sudo or silently weaken test expectations.

T1 targeted assessment: inspected client send_leave and shared endpoint QoS. Leave writes to the correct writer, then waits one second before listener removal. Reliable Leave uses matching endpoint settings, bounded KEEP_ALL32, max blocking100ms and automatic liveliness3s. Re-read existing ack-resume-network1 evidence: matched readers1 before/after; ACK returned-10 after1000571819ns; leave_ack Timeout; suite exit1. Server recorded identity6 Leave, but receipt does not prove transport ACK completion. No justified application/QoS repair identified. Production code and test expectations unchanged; no new game runtime or rebuild in this slim assessment. Prior goal turn made progress by preserving diagnostic evidence, not by fixing ACK.

STOP: awaiting T1 approval. Proposed fallback ONLY for DDS_RETCODE_TIMEOUT after a successful Leave write: emit a clear warning and allow normal client exit0. Other write/ACK/cleanup failures remain failures. Tests must independently establish that the specific session's slot is freed using its explicit Leave audit or observed lease expiry, plus successful replacement admission. Document native transport acknowledgement as a known limitation; never label the timeout fixed or transport reliability passed. Do not implement this proposal before approval.

Remaining slim tasks after approval: T2 clean clone without keys, make/test/hardening, two-client headless and strict GUI; T3 evidence document from existing results plus course perf commands, then approval gate; T4 capture helper check and human sudo capture followed by unprivileged analysis; T5 README/limitations/AI log/handover/source ZIP exclusions. TSan/Valgrind dependency findings, two-computer test, further GUI investigation, slides and video are out of scope and remain disclosed limitations. The new clock-witness diagnostic is preserved but has not been run and is not part of this finish plan.

---
# Teammate handover - start here

**Goal PAUSED by Stanley on4November2026 because usage was running out.** This is an unfinished, tested checkpoint, not a submission-ready release. Publishing this checkpoint is explicitly authorized by Stanley; no further game investigation was performed after pause.

## Current state in one minute

- Contributor main commit2cad2e1 (server-authoritative Asteroids/security/tests) was fast-forwarded into this checkout. Our earlier P0 work was preserved under docs/history/local-p0; the original local work also has an external task-workspace backup. Do not rebuild the game from the old skeleton.
- New listener-based I/O fixes the observed GUI lease/state regression in the latest targeted run: both clients stayed CONNECTED / DDS SECURE, fresh own-state streams, two distinct ships, asteroids/HUD, real movement/fire, timeouts0 and invalid0.
- **Overall network and GUI checks still FAIL:** a client can report `leave_ack: Timeout` and exit1 during shutdown. The server receives/audits Leave, but native `dds_wait_for_acks` does not complete successfully. Do not suppress this or call the run passing.
- Marker measured requested1,000,000,000ns, elapsed1,000,082,330ns, returned-10. This is a genuine one-second ACK deadline expiry, distinct from the older~4.75-second waitset overshoot.
- No server/client/Xvfb process remained at final cleanup verification. No working desktop session is promised to still be open. No keys, captures or compiled binaries are committed.

## What changed after contributor main

1. `src/common/event_queue.{h,c}`: bounded18-bit reader notification handoff, pthread mutex plus condition explicitly CLOCK_MONOTONIC,50ms timer. Stable data-available listener arguments, startup seed notifications, full-batch requeue after loan return. No DDS/audit work under queue lock; synchronous listener detach/checked participant delete precede queue destruction. Unsafe cleanup failures exit nonzero. Client/server keep their existing main+worker split, native security, QoS, IDL and3-second lease. Old waitset attachment helper removed.
2. `tests/test_event_queue.c` and Makefile test target: real queued/coalesced wake, invalid tags, requeue, asynchronous producer,50ms idle timeout and teardown checks. Build and unit target exited0; measured idle50,138,723ns. C compiler clean; make printed host timestamp/clock-skew warnings, retained in evidence.
3. Audit safety: `audit_open` refuses existing/symlink targets; default paths are unique. Regression proves old contents survive rejection. Explicit audit paths must now be fresh.
4. Harness evidence safety and acceptance: P1/GUI accept `--output-dir` and refuse old directories. Fault checks wait for admission before validating rejection. GUI now rejects unexpected timeouts/invalid sessions, duplicate IDs, stalled own-state streams and stale final progress. Earlier GUI PASS with timeouts2 was a false-positive harness result and is not accepted.
5. `tools/play.sh`, `stop.sh`, `game_session.py`: secure managed launch by default; scoped pidfd shutdown of the exact owned supervisor, duplicate refusal, sequential admissions, unique session logs. `play_local.sh` supports an explicit private bundle. `--baseline` explicitly selects preserved P0 tooling; never silently downgrade security.
6. Diagnostics only: `dds_call_markers.c`, `dds_wait_probe.c`, `gui_runtime_diagnostics.py`. These do not belong in normal production execution. Marker preload changes thread names/timing observation only and must never preload helpers/Python or expose sample/key/pointer data.

## Evidence: passes versus failures

Selected exact output is committed under `docs/evidence/2026-11-04-handover/`; full local logs/PNGs/JSONL remain in ignored `build/evidence/` and are not portable unless separately shared.

| Check | Actual result |
|---|---|
| Merged release build, protocol/sim units, baseline build, hardening | Passed; baseline warnings and host clock-skew warnings disclosed |
| Listener build + new queue units | Passed, no C compiler warnings |
| Earlier headless integration run3 | Passed12joins/1reject/11leaves/exactly1intentional timeout and audit verification; earlier shutdown hang remains historical/unexplained |
| Listener networking run1 | FAILED spoof-client shutdown ACK timeout after admission/cap/crash/spoof checks passed |
| Listener strict GUI run1 | FAILED only during shutdown ACK; active-state guards passed, joins2/peak2/timeouts0/invalid0/leaves2, inputs231/snapshots268/moves1114/shots11 |
| ACK-instrumented networking | FAILED same one-second ACK expiry, with server Leave receipt and clean summary |
| Full P4 TSan/Valgrind | NOT PASSED; contributor retains OPEN HIGH dependency findings |
| External protected-payload capture, LAN, full10-minute soak, P5/P6 | Outstanding |

### Why listeners were introduced

With attached game readers, native `dds_waitset_wait` requests50ms took~4.47-4.78seconds. Empty-guard idle probe12pairs and60pairs behaved~50ms, so a generic unit/clock error is not established. Exact Cyclone0.10.4 waitset implementation includes mutex work outside its condition deadline. The underlying internal cause is not proven. Lowering Mesa LP_NUM_THREADS74->14threads reduced rendering cost but did not prevent lease loss. The approved listener design avoids that application wait path without polling, a new thread, weaker crypto, longer leases or changed wire format. Other native timers and P4 findings remain open.

## Reproduce on your machine

Use Ubuntu/WSL in your own checkout. Do not copy Stanley's `/home/stanley` or old `/tmp` paths. Earlier /tmp credentials disappeared between runs, so prefer persistent private Linux-home storage. Never generate credentials on `/mnt/c` or commit/export the full CA fixture.

```bash
make -j2
make test
make check-hardening
umask 077
mkdir -p "$HOME/.local/state/dds-asteroids"
bundle="$HOME/.local/state/dds-asteroids/credentials-$(date +%Y%m%d-%H%M%S)"
bash tools/gen_certs.sh "$bundle"
# Use new output names for every attempt; never overwrite earlier failures.
python3 -B tests/p1_network.py --security-dir "$bundle" --output-dir build/handover-network-1
```

Strict graphical reproduction, **without** preload/thread caps:

```bash
env -u LD_PRELOAD -u LP_NUM_THREADS xvfb-run -a -s '-screen 0 1620x650x24'   python3 -B tests/p3_gui.py --security-dir "$bundle" --output-dir build/handover-gui-1
```

ACK diagnostic (test process children only):

```bash
cc -std=gnu11 -Wall -Wextra -Wformat=2 -Werror -fPIC -shared   tests/dds_call_markers.c -o build/dds_call_markers.so   $(pkg-config --cflags --libs CycloneDDS) -ldl
python3 -B tests/gui_runtime_diagnostics.py --suite network   --dds-markers "$PWD/build/dds_call_markers.so"   --diagnostics build/handover-ack-1.jsonl   --security-dir "$bundle" --output-dir build/handover-ack-1
```

After tests actually pass, desktop/manual session:

```bash
bash tools/play.sh --security-dir "$bundle"
bash tools/stop.sh
```

## Next investigation (do this first)

1. Read `src/client/net_client.c:send_leave`: native write then writer-specific `dds_wait_for_acks(...,DDS_SECS(1))`, before listener detach/deletion. It passes the correct writer, not implicit publisher. No direct new teardown race has been identified.
2. Determine why the matched reliable reader ACK is missing/delayed. Server application receipt is not equivalent to acknowledgement of every matched reliable reader. Inspect supported native matching/ACK evidence; retain clocks and return values. Do NOT extend the deadline, ignore errors or weaken tests merely to obtain green output.
3. Review whether a separately approved application-confirmed Leave protocol is needed only after diagnosing native behavior. Snapshot omission alone is not general session-specific confirmation because a slot can be reused rapidly. Any breaking IDL/policy change requires explicit user approval.
4. Rerun relevant headless and strict GUI checks without the shim after a real fix. Both clients must remain connected, recent, responsive and leave normally. Inspect final PNGs. Do not clear the earlier intermittent server hang or P4 issues from one successful run.

## Remaining assignment work

- Resolve native/dependency TSan races/mutex lifetime and Valgrind uninitialised crypto-send bytes. No suppressions/blanket false-positive dismissal. A matching instrumented native dependency investigation needs CMake, absent on this machine. Human command: `sudo apt update && sudo apt install -y cmake`. Agents never run sudo; installation is a prerequisite, not a promised fix.
- Full ten-minute5-client sanitizer churn, clean-clone/source-ZIP reproduction, actual two-computer/VPN reachability/provisioning/gameplay, latency/bandwidth/loss evidence and authorised external encrypted-payload capture. Current config is domain42/loopback. Do not weaken firewalls or disturb unrelated WSL work.
- P5 headless benchmarks/PERFORMANCE.md and measured compiler/branch/cache/dependency/SIMD/algorithm/idleCPU results. Prepare short course Linux perf commands; label local clock/Cachegrind separately.
- P6 slides/PDF, diagrams/evidence tables/AI declaration, source ZIP, walkthrough/defence, human playtest, peer evaluation and real team-narrated MP4 <=5minutes/200MB. No AI voice.
- P0 capture script currently has P0/truncated-capture limits; update/review it before claiming complete secure-game packet evidence. User runs sudo capture; agent analyses pcap without sudo.

## User instructions and rubric that still govern

- P0 approved; native DDS SecurityA approved; integration of main/v2/P1 authorized. Current GUI investigation was explicitly authorized by `FIX: continue GUI investigation`; no `APPROVED P1` yet. Goal now paused, not complete.
- After each phase: actual tests, safe play/stop tools, open/confirm server+two WSLg clients, numbered human checklist, STOP until APPROVED P<n> or FIX. Max3repair loops then report blocker; no automatic gate bypass or next-phase acceptance. Preserve no-sudo, no keys in source/logs/ZIP and evidence honesty. The user's explicit checkpoint push overrides older no-commit/push instructions only for this publication.
- Team Lab PDF tasks/rubric are tracked in PDF_REQUIREMENTS.md: program40%, slides20%, video20%, peer evaluation20%; optimization/security40% of program mark. Deadline in supplied brief Friday6November2026, not independently checked on Moodle. No grade guarantee/all-standards-met claim.
- Stanley Lur is leader; Jordain Ng member; no team name. They will provide actual responsibilities; do not invent contributions.
- Current planning: INTEGRATION.md, TODO.md, PROGRESS.md. Earlier contradictory BLOCKED/PASS statements below current headings are preserved history. This handover and its evidence are the latest snapshot.


## Approved T1 fallback verification - 5 November 2026

User approval: "Approved T1". Separate review confirmed only DDS_RETCODE_TIMEOUT following successful Leave write is downgraded to an explicit warning. Write errors, other ACK errors and cleanup errors remain fatal. No security, QoS, lease, wire-format or timeout changes. The native ACK problem remains a known limitation.

Actual incremental client compile exit0; full compiler output: build/evidence/slim-t1-build.txt. Targeted native lifecycle run, no preload/thread cap: tests/p1_network.py --security-dir <private Linux bundle> --output-dir build/evidence/slim-t1-network1; exit0. Output includes PASS fresh server slot release per identity, PASS final fault slot permits replacement, SERVER_SUMMARY joins13 rejects1 leaves12 timeouts1 invalid4 stale1 peak5, and PASS lifecycle audit records36. One timeout is the intentional SIGKILL lease test. Complete output and exit file retained; this run did not emit the warning and alone did not exercise the fallback branch.

Explicit test-only public API fixture tests/force_ack_timeout.c simulates DDS_RETCODE_TIMEOUT, without probing internals. Compiled with cc -std=gnu11 -Wall -Wextra -Werror -fPIC -shared; isolated tests/ack_probe.py --dds-markers build/force_ack_timeout.so --output-dir build/evidence/slim-t1-forced-timeout exited0. Actual output: CLIENT_WARNING leave_ack_timeout: transport acknowledgement unconfirmed; server slot release requires independent verification; CLIENT_EXIT; server_leave_observed true; audit_leave_observed true; client_cleanup_exit0; server_cleanup_exit0; status PASS. This establishes application handling of an injected timeout, not native ACK reliability. Full output: build/evidence/slim-t1-forced-timeout-output.txt.

T1 approved fallback implemented and targeted checks passed; T2 reproduction pending. Existing unsuppressed dependency findings and all historical failures remain disclosed.


## T2/T3 slim checkpoint - 5 November 2026

T2 fresh clone plus current working patch: build/unit/hardening exit0; headless124.539seconds after both admissions, joins2/leaves2/timeouts0/invalid0, audit verified; strict GUI exit0, both new final screenshots inspected once with connected HUDs, ships and asteroids. Not a clean published final revision. Complete nonsecret output copied into docs/evidence/2026-11-05-slim-finish/. T3 docs/EVIDENCE.md uses existing results only, labels historical P2 results, records missing idle CPU/profiler data and all excluded work as limitations. Exact course perf commands prepared with explicit missing-binary prerequisite. STOP at T3 approval gate; no T4 capture work or T5 packaging yet. Native ACK reliability remains unproven despite approved application fallback. No new sanitizer runs or benchmarks.


## Slim final-documentation reconciliation - 5 November 2026

T3 approval is recorded. README current limitations and known team roster, EVIDENCE status, AI-use record and RUN_AND_VERIFY handover are reconciled. Source-ZIP instructions explicitly require human review/commit, forbidden filename check and archiving the reviewed revision; no archive has been generated or submitted. Exact requested live capture path /home/stanley/.local/state/dds-asteroids/captures/secure-manual-20261105-01.pcap was checked: Exists False; Bytes not available. No new live capture, game run, build or test rerun. T4 analysis cannot proceed without the actual human-generated file/path. T5 documentation/instructions are prepared, subject to adding the real capture outcome and final evidence audit. No full-goal completion claim.


## User-requested automatic launcher and lecturer checklist test - 5 November 2026

See REQUIREMENTS_CHECK.md and docs/evidence/2026-11-05-requirements/. Real play_local.sh --duration45 under isolated software-GL Xvfb passed two admissions/windows, real key input, fresh shared state, first-client Leave and session exit0: joins2 leaves2 timeouts0 invalid0 peak2 moves1627 shots14 snapshots310. Both new final screenshots inspected once. Actual WSL and OpenGL/GLFW/DDS linkage confirmed; no remaining game/Xvfb process rows.

Targeted authentication first attempt FAILED due to my identity2 fixture selection: rogue_client2.crt missing, so no server auth-warning audit. Preserved and reported. One test-setup correction selected generated rogue identity6: wrong-ca and no-cert rejected exit1 while a legitimate client stayed alive; no extra slots, auth_failures6, server audit verified, test exit0. No product/security/assertion change.

Approval is limited to tested launcher/local multiplayer/certificate rejection. Cross-machine operation and external encryption capture remain unverified; cached-glyph/condition-wait concepts are confirmed but speedup/idleCPU is unmeasured. Known ACK/sanitizer limitations retained. No full-suite or sanitizer/benchmark rerun and no agent sudo. User's full requirement list is not yet approved.
