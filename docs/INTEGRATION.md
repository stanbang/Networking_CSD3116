# Paused checkpoint - latest authoritative status

Stanley explicitly paused the goal on4November2026 and requested a teammate handover plus publication. Read HANDOVER.md first. The listener repair preserves active two-client GUI state in the latest targeted run, but overall GUI/network checks still FAIL native one-second Leave ACK timeout. No further game tests/repairs after pause. Selected portable raw evidence is in docs/evidence/2026-11-04-handover/. Earlier status text below is historical. No goal-complete or submission-ready claim.

---
# Latest integration status: GUI verification blocked

See the current blocker section below. Existing build/unit/headless passes are narrower than full graphical multiplayer acceptance.

# Main integration and remaining work

Purpose: reconcile the contributor merge with the existing assignment plan. CSD3116 team with declared AI assistance, 4 November 2026.

## Integrated source

Fast-forwarded main from0168c3e to2cad2e1, the existing contributor commit "Implement server-authoritative multiplayer Asteroids". No new commit/push was made. It adds/changes54files, approximately5436added/170removed lines. All prior local changed/untracked bytes were backed up to the task workspace at pre-main-20261104-120444. Four colliding local P0 reports are preserved under docs/history/local-p0/. Other local work was retained, including the PDF evidence map and approval workflow. Baseline and third-party source bytes were not edited by this integration.

## What was added versus what is verified

| Area | Added in main | Fresh local evidence / remaining limit |
|---|---|---|
| Game | Authoritative60Hz co-op ships, bullets, splitting asteroids, waves, lives/respawn, game-over/restart, interpolation, HUD,800x600 | Source present; simulation unit tests pass. Contributor gameplay/GUI reports remain historical until rerun here. |
| Protocol/lifecycle | Versioned bounded IDL, addressed nonces, per-identity channels, leave/3s lease, five-slot cap, finite CLI/bots/faults | Fresh P1 run3 passed12joins/1reject/11leaves/exactly1expected timeout, spoof/session/idzero/idhigh/replay drops and audit-chain verification. Run1 timeout/shutdown hang is retained as unresolved intermittent evidence. |
| Security | Native X.509 authentication, signed exact-topic permissions/governance, encrypted DDS, rate limits, chained audits, actual ciphertext-fault harness | User approved nativeA; code is present and secure clients joined in local P1 test. Full fresh P2 negative/capture acceptance is still required. Plugin presence/source inspection alone is insufficient. |
| Build/hardening | Separate baseline bindings, grouped IDL generation, hardened release, ASan/TSan targets, enforcing hardening checker | Release/unit/baseline/hardening passed locally. Original baseline unused-argument warnings and initial host clock-skew warnings are disclosed. No current sanitizer-clean claim. |
| Tests/docs | Protocol/sim/network/security/gameplay/game-over/GUI/soak/Valgrind harnesses; threat model, playtest, hardening, review/test reports, TODO | Tests existing is not all tests passing. Imported reports from contributor's machine remain labelled historical. |

## Newly corrected in this integration

- Audit log opening used truncating fopen on a fixed default path, following symlinks and overwriting prior evidence. Explicit log paths now use exclusive no-follow creation; omitted paths get a unique name. A real unit regression proves duplicate/symlink attempts preserve old contents.
- P1 harness now accepts a fresh --output-dir and refuses existing evidence. Fault clients no longer finish after only2seconds during certificate admission; the harness waits for real admission, checks each real drop/audit event, then stops the client. It now asserts exactly the single intentional SIGKILL timeout rather than allowing arbitrary legitimate-session loss.
- The first rerun was blocked because the earlier /tmp credential fixture disappeared. The replacement is owner-only persistent Linux-home storage, with mode0700 directory/0600 keys. No keys are stored in repo/logs/reports. The failed prerequisite attempt remains evidence, not a pass.
- Secure play/stop entrypoints are being verified against the existing contributor launcher. Baseline access is explicit --baseline; there is no insecure fallback. Each game session retains unique audit/stdout logs and admits clients sequentially.

## What remains missing

1. Diagnose the locally observed intermittent server/network shutdown hang. The later successful run does not establish its cause or eliminate it. Preserve it alongside the known dependency findings.
2. Resolve the contributor's OPEN HIGH TSan races/mutex-lifetime reports and Valgrind uninitialised crypto-send bytes. Do not suppress them or declare dependency reports false positives without proof. CMake is absent here; human prerequisite for matching dependency investigation: sudo apt install -y cmake. No sudo/install was run.
3. Full10-minute five-client sanitizer churn; only contributor120-second ASan churn is reported. Literal clean-clone and extracted submission-source builds remain required.
4. Fresh full security-negative suite, external authorised encrypted-game capture, cross-machine reachability/provisioning/GUI/gameplay, latency/bandwidth and controlled loss/delay evidence. Loopback and packet signatures do not prove all this. Existing P0 capture helper's256-byte truncation is not sufficient for complete secure-protocol evidence.
5. P5 headless benchmarks, before/after compiler/branch/cache/dependency/SIMD/algorithm/idleCPU measurements, PERFORMANCE.md, and short course-server perf commands. Fixed pools and cached glyphs alone do not complete this requirement.
6. P6 presentation PDF, design diagrams/evidence tables, five-minute<=200MB team-narrated MP4, source ZIP excluding keys/artifacts, AI declarations in every deliverable, code walkthrough/defence, human playtest and peer evaluation. Actual contributions must come from the team.

## Schema reconciliation

Retain the merged implemented Ast schema rather than rebuilding it from the earlier draft: bounded typed ships5/bullets128/asteroids64 are present. It differs from that draft in64-bit session tokens, no explicit server-instance epoch, no dedicated heartbeat/auth_blob (input provides heartbeat; native authentication is out of band),16-bit IDs/status, fixed-point signed coordinates and strict monotonic application sequence rejection. These are documented differences, not claims of exact draft equivalence. Native permissions bind exact identity-specific topics; a payload ID alone is never authority. Known64sessions-per-identity lifetime cap and absent automatic server-restart recovery remain explicit limitations. Do not change the IDL further without the user's separate breaking-change approval.

## Reconciled phase order and gates

| Phase | Current treatment |
|---|---|
| P0 | Locally verified and user approved; historical baseline evidence preserved. |
| P1 | Reuse merged implementation. Fresh networking/unit checks pass after harness/evidence fixes, but prior hang remains open. Complete secure launch controls/manual P1 gate; no automatic next-phase acceptance. |
| P2 | Existing native-security implementation and prior contributor tests; independently revalidate after P1 approval. NativeA is already selected by user. |
| P3 | Existing gameplay/rendering, not absent. Verify and fix concrete gaps after its phase gate; do not recreate working code. |
| P4 | Partial, not passed: known TSan/Valgrind findings, full soak/dependency investigation. |
| P5 | Missing measured optimization artifacts. |
| P6 | Missing final submission artifacts and human contributions/recording/evaluation. |

The contributor TODO's statement that two-computer work was paused is historical context, not a new instruction from this user. Cross-machine evidence remains an outstanding requirement; do not modify host networking/firewalls or unrelated WSL work speculatively. Keep the user's manual APPROVED P<n>/FIX gates and current goal scope.

## Grade and team contract

The Team Lab PDF remains the grading checklist: program40%, slides20%, video20%, peer evaluation20%; optimization/security40% of program mark. The original PDF deadline is Friday6November2026, confirmed from the supplied brief, not from Moodle. Stanley Lur is leader, Jordain Ng member, no team name; responsibilities will be supplied by them. No grade guarantee or all-standards-met claim is supported. The PDF_REQUIREMENTS matrix must be closed with actual evidence, including final slides/video/ZIP, before completion.

## Evidence and role honesty

build/evidence/integration-build-*.txt: independent original builder build/unit/baseline/hardening.
build/evidence/integration-p1-network-output.txt and integration-p1-network-logs/: original independent tester failed run1.
build/evidence/integration-p1-run2/: missing temporary-credential prerequisite failure.
build/evidence/integration-p1-run3-output.txt and integration-p1-run3/: fallback tester passing run3.

The real builder/tester reached usage limits and the agent tree became unavailable. SUBAGENTS_UNAVAILABLE was declared. Later fixes/review/tests use sequential root roles with a fresh actual-diff reread; they are not misrepresented as independent subagent verification. Existing real reviewer findings and all failed evidence are retained. Maximum3integration attempts used; do not silently repeat another failing loop or erase the first hang.


## Current blocker: real secure GUI acceptance failed

Actual `xvfb-run ... tests/p3_gui.py --security-dir <persistent Linux bundle> --output-dir build/evidence/integration-gui-run1` exited1. Both windows rendered, but the server summary had joins2, peak1, timeouts2, inputs5, moves0, shots0, snapshots40, ticks1500. The first lease expired before the second player joined. Both final PNGs show WAITING FOR GAME STATE rather than a current connected two-player world. Root inspected both actual images. This contradicts accepting the merged game as working locally on this host, regardless of contributor historical reports. It is not cleared by the headless run3 pass.

Evidence: build/evidence/integration-gui-output.txt, integration-gui-exit.txt, integration-gui-run1/*.log and client1-10s.png/client2-10s.png. Cleanup completed; no successful manual GUI session is currently left open. Do not fabricate a manual-phase pass or open the game as if all own checks passed.

The current bounded integration attempt cycle is stopped at the original maximum-three-attempt rule. The middle missing-/tmp-bundle prerequisite failure is disclosed, not silently erased to extend the limit. Request a human decision to authorize a new targeted GUI/network starvation/hang investigation cycle. No fourth network attempt or second GUI attempt has been run. A diagnosis is not established: do not label native dependency, GPU, clock, or the harness as the cause without stronger evidence.

The merged implementation and all local work remain preserved. P1 integration acceptance is INCOMPLETE despite passing headless functionality. Native-security and P3 source exist; P2/P3 have not been newly accepted here. P4 OPEN HIGH TSan/Valgrind findings, full soak, external capture, two-machine verification, P5 measurements and P6 deliverables remain required by the original scope/PDF checklist.
