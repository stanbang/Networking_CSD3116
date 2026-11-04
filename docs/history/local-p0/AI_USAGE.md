# AI usage declaration

Purpose: record assistance and its verification. CSD3116 team, 4 November 2026 (Singapore).

This project uses Codex assistance. The team remains responsible for understanding, testing and defending accepted output. No AI narration may be used for the final video. The declaration must also appear in the presentation and video.

## Session: P0 preparation and audit

User supplied two pasted prompts specifying a secure multiplayer asteroid game, phased builder/reviewer/tester workflow, and 14 course PDFs. Both prompt files were read. Repository `AGENTS.md`, `CODEX_GOAL.md` and `docs/PROMPT.md` were read. The Team Lab PDF was extracted in full; relevant pages from the lecture decks were extracted and mapped to acceptance evidence. Assignment pages 14, 16, 18, 19 and 20 were rendered and visually inspected.

Three real subagents ran: builder (forced unchanged baseline compilation), reviewer (independent source/security/threading audit), tester (independent environment and tracked-key-path checks). The orchestrator consolidated their actual results. No production game code was generated, and no source changes, commits, pushes or git configuration changes were made. Existing baseline changes were preserved.

Generated documentation: `PROGRESS.md`, `TEST_REPORT.md`, `REVIEW_LOG.md`, `PDF_REQUIREMENTS.md` and this declaration. PDF extraction/inspection intermediates remain outside the source checkout. Reference PDFs were not modified or copied into the submission.

Verification: `make -B baseline` exited 0 with inherited warnings; environment versions, security plugin paths, ASLR=2 and no tracked key/pem paths were observed. Missing GUI/capture tools invoke the explicit no-sudo/stop rule. No graphical runtime, gameplay, security rejection, sanitizer, Valgrind, packet-capture or performance claim is made. See the report files for raw output and exact missing checks.

User clarified: no team name; Stanley Lur is leader; Jordain Ng is second member; the team will supply responsibilities. No member contributions have been fabricated.

Rejected/uncertain output: the original installation request included both tcpdump and tshark, already-installed xauth, and optional xdotool. It was corrected to the minimum `xvfb imagemagick tcpdump`. The reviewer also rejected treating the server's unlocked active-count read alone as proof of a data race, because it found no competing worker access. Such corrections must remain visible when defending AI-assisted output.

Phase acceptance: P0 incomplete. P1-P6 not started. Team review/defence of this documentation is pending.


## Resumed P0 workflow update

The user reported installing GUI/capture tools, required real desktop play sessions and explicit approval between phases, and required separate approval before security selection or breaking IDL changes. The three existing agents were resumed. Tool checks found xvfb/import/tcpdump present but xdotool still absent; this discrepancy was reported. Native security Option A is now a recommendation pending approval, not a selected mechanism. No IDL or baseline source changes were authorized or made. P0 launcher/capture and GUI-test tooling is being prepared and must be reviewed/tested before any passing claim. Historical failed preflight output is retained.


## P0 tooling and actual GUI completion

Generated AI-assisted P0 play/stop lifecycle scripts, bounded human-run capture script, automated GUI harness and P0 runbook. Builder implemented tools; reviewer independently audited process/capture safety and evidence ownership; tester independently executed safe checks and actual software-GL two-client test. Review findings were fixed in tools without changing baseline/third-party/IDL.

xdotool became available during the session. Actual GUI run1 failed on black capture of an obscured OpenGL window. The failed output was kept; a narrow raise/repaint change fixed screenshot acquisition. Actual run2 exited0: each intended square moved, client views matched, lifetime11.087s. Tester, reviewer and primary inspected images. The primary launched server/two clients through WSLg, verified visible windows/PIDs on display:0 and inspected screenshots. Exact output is in TEST_REPORT.

P0 automated checks pass for the insecure square-demo scope only. User manual approval is pending; no next phase may start until approval. No authentication/encryption/asteroid/sanitizer/performance/capture pass is claimed. No sudo, source commit, push, baseline edit or IDL change occurred. Team responsibilities remain for the team to supply.

The primary additionally verified duplicate-session refusal, actual managed three-process stop, no remaining baseline windows/processes, and successful relaunch. Final WSLg windows are left open for the manual P0 gate; no further edits/commands are allowed until the requested approval/fix response.

User approved P0 and reported a zero-packet human capture. The agent inspected sockets unprivileged and identified the fixed-port assumption as insufficient for observed ephemeral DDS sockets. Three real subagents resumed for a capture-only correction, independent review and offline filter validation. No real capture, security selection or breaking IDL change is inferred or approved.

Capture-only correction used real builder/reviewer/tester. Actual offline filter tests passed; raw output preserved, real capture pending. Builder mistakenly reused a lifecycle guard driver and stopped the user demo despite the no-stop instruction. The primary disclosed the mistake and restored/confirmed both WSLg windows. No baseline/IDL changes or security selection occurred.


User explicitly approved native DDS Security A and emphasized exact Team Lab grading compliance. Primary reread PDF tasks/deliverables/rubrics pages14/16/18/19/20 and confirmed mapped weights/requirements. Security choice is now authorized, not implemented. Final concrete P1 schema proposal updated with widths/bounds, separate v1 bindings and native-security identity/topic boundary. Actual IDL remains unchanged awaiting separate breaking-change approval. No grading requirement is waived; current project is not yet compliant with all PDF requirements.
