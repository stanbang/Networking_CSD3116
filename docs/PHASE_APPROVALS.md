# Phase approval workflow

Purpose: preserve the user's direct instruction update of 4 November 2026 (Singapore). Maintained by the CSD3116 team with Codex assistance.

This workflow supersedes conflicting automatic-advance/default-decision text in the original prompts. Keep the existing P0-P6 plan, work and real builder/reviewer/tester subagents.

1. Recheck installed GUI/capture tools. Required GUI smoke uses `xvfb-run`, `xdotool`, ImageMagick `import` and software GL. Never infer a pass from package installation alone.
2. For each phase, complete builder work, independent review and actual automated verification. Paste real output in `TEST_REPORT.md`. Add/update `tools/play.sh` and `tools/stop.sh` for that phase.
3. Once the phase's own tests pass, launch the server first, then two separate clients through WSLg and confirm the windows are open. If the environment blocks GUI/background processes, give exact commands for the user's terminals and honestly record the unverified launch.
4. Print a numbered manual test checklist and STOP. Do not start the next phase, edit code or run more commands until the user replies `APPROVED P<n>` or `FIX: <what to change>`. A `FIX` authorizes correction in the current phase, followed by renewed review/tests/manual gate.
5. Before choosing a security mechanism, provide the actual plugin-check result, options and recommendation, then wait for explicit security approval. Prior Option A language is a recommendation only. Phase approval alone is not security selection approval.
6. Before any breaking change to `idl/messages.idl`, present the concrete proposed change and wait for explicit approval. Preserve baseline bindings independently if v2 later breaks compatibility.
7. Provide `tools/capture_demo.sh`; the user runs it with sudo. The agent never invokes sudo. Analyse the resulting `.pcap` without sudo. Ignore `*.pcap` in git. Capture is required evidence, but a script's existence is not capture evidence.
8. Never mark a check PASS unless it ran and its actual output is preserved. Screenshots prove only what they actually show. Baseline traffic is insecure and must not be described as ciphertext.

## Current gates

- Current phase: P1 integration/revalidation of contributor implementation. P0 approved; current live window state must be checked, not inferred from historical P0 notes.
- P0 APPROVED by the user: "Approved. instructions work and terminal captured this" (4 November 2026). Natural-language approval accepted; no approval of security choice or breaking IDL is implied.
- Security choice APPROVED: user replied "A. make sure it follows this pdf specifications exactly . this rubric decides my grade" on4 November2026. Native DDS Security selected; not implemented or verified yet.
- User authorized integration of merged main and proceeding with v2/P1. Existing merged Ast IDL retained; differences from the earlier draft documented in INTEGRATION.md. Further breaking IDL changes require approval.
- GUI tools: all required tools are now present, including xdotool at /usr/bin/xdotool (dpkg1:3.20160805.1-5build1). Initial missing-package checks are historical.
- Human loopback capture attempted: zero packets, no file produced, NOT PASSED. Filter correction passed offline IPv4 tests; actual retry capture still pending. Offline validation is not real traffic evidence.

Record phase approvals here and in `PROGRESS.md` when they arrive, with the exact user instruction. Do not turn an automatic goal continuation into a manual approval.


Current P1 integration gate: incomplete after secure GUI failure; no APPROVED P1 received. Own checks are not all passing, so no manual acceptance checklist is issued as a successful phase. Await authorization for a new bounded repair cycle under CODEX_GOAL loop limit. No next phase starts automatically.
