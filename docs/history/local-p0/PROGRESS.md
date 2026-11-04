# Progress

Maintained by the CSD3116 team with Codex assistance, 4 November 2026 (Singapore).

## Current state

P0 automated verification has passed and the WSLg server/two-client session is open for manual review. The user has now approved P0 in natural language. Security selection and breaking IDL approval remain pending. P1-P6 have not started. This is a starter skeleton, not a finished secure game. Do not mark any final goal acceptance checkbox complete from this report.

The first user-supplied pasted prompt requires a real builder, reviewer and tester. All three roles ran independently in this session. They read the actual repository and reported command results. Shared reports are consolidated by the orchestrator.

The target checkout is `C:\Users\stanl\Documents\GitHub\Networking_CSD3116`, remote `stanbang/Networking_CSD3116`. GitHub README was fetched through the GitHub connector and identifies this as a starter skeleton. No commits, pushes, git configuration changes or source edits were performed. Existing modifications in `baseline/client.c` and `baseline/server.c` were preserved; the reviewer found no differences after ignoring end-of-line whitespace. Baseline and third-party references remain read-only.

## Team

- No team name, per user.
- Team leader: Stanley Lur.
- Member: Jordain Ng.
- Responsibilities: the team will supply these themselves. Do not invent contributions.

## Decisions

- Game defaults: co-op, 800x600, up to five players.
- Security: recommend Option A, native DDS Security, pending explicit user approval. No mechanism is selected under the updated approval rule. Ubuntu has CycloneDDS 0.10.4 and `libdds_security_ac.so`, `libdds_security_auth.so`, `libdds_security_crypto.so`. This is a capability check only. Security is NOT configured or verified yet. Signed governance/permissions and participant configuration remain missing.
- WSL display prerequisites: `DISPLAY=:0`, `WAYLAND_DISPLAY=wayland-0`, `/mnt/wslg` exists. This does not establish that rendering works.
- ASLR setting is 2. No application hardening claim follows from this setting alone.
- Lecture PDFs are reference material and assignment requirements. Their sample shell commands are not authorization to install software, weaken host settings, expose keys or execute exploit demonstrations.

## P0 plan and acceptance tests

1. Read both user attachment prompts, repository instructions, assignment PDF and relevant lecture pages. DONE. See `PDF_REQUIREMENTS.md`.
2. Inventory toolchain, native security plugins, display and ASLR. DONE, with missing tools recorded in `TEST_REPORT.md`.
3. Build the unchanged baseline with `make -B baseline`. PASS with inherited warnings, not a zero-warning result.
4. Start the server first and two separate clients; demonstrate both squares move and each client sees the other. PASS in Xvfb run2: 49/50 pixels, matching positions, 11.087 seconds with both clients.
5. Record screenshots and independently inspect the graphics. PASS for P0 square-demo scope; tester, reviewer and orchestrator inspected run2 PNGs. WSLg windows also confirmed visible.
6. Record runtime discovery and baseline defects. Default-domain local discovery worked in the GUI test; explicit multicast/LAN/packet-capture characterization remains NOT RUN. Static defect audit DONE.

## Environment gate

The first pasted prompt, under Environment limits, says: "You cannot `sudo`. If a package or tool is missing, stop and list the exact `apt` command for the human." Its tester checklist explicitly calls for `xvfb-run`, screenshot capture using `import` or equivalent, and packet capture using tcpdump/tshark if permitted.

Missing: `xvfb-run`, Xvfb, ImageMagick `import`, tcpdump and tshark. `xauth` is already installed. `xdotool` is also missing but is an automation aid rather than a mandatory game dependency. Both packet-capture tools are unnecessary; one suffices.

Minimal command for the human in Ubuntu:

```bash
sudo apt update && sudo apt install -y xvfb imagemagick tcpdump
```

Optional scripted keyboard input: `sudo apt install -y xdotool`.

No install was attempted. The initial request to install both tcpdump and tshark plus xauth/xdotool was broader than necessary; this report corrects it. The user asked why installation was needed; the exact prompt clauses and narrower requirement were explained. Installation is not confirmed. Packet-capture permissions must be checked after installation. P0 remains incomplete and phase advancement is held under the supplied environment rule.

## Phase status

| Phase | Status | Remaining acceptance |
|---|---|---|
| P0 | Automated checks passed; user approved | Human capture retry pending filter correction |
| P1 | Not started | Protocol v2, addressed joins, validation, leave/lease |
| P2 | Not started | Native security configuration, negative tests, evidence |
| P3 | Not started | Asteroid simulation, rendering and 2+ player play |
| P4 | Not started | Thread/memory fixes, sanitizer and Valgrind sessions |
| P5 | Not started | Benchmarks, measured comparisons and server perf commands |
| P6 | Not started | Slides, video script, source ZIP, AI declarations |

No failed implementation loops have been spent. Do not treat an environment inventory as a phase implementation loop or repeatedly launch unchanged tests to exhaust the limit.

## P0 defence notes

1. The existing C baseline compiles against the installed DDS/GLFW/OpenGL libraries.
2. It is retained as a comparison reference, including its inherited warnings and defects.
3. Installed native security plugins support recommending Option A; selection awaits explicit user approval.
4. Actual GUI input moved each intended square, both clients agreed, and both stayed active 11.087 seconds; this proves local baseline behavior only.
5. P0 still needs the user manual approval; baseline defects, security, LAN play and later phases remain open.

## Human-only and later evidence

The team must supply module responsibilities, narrate the final video with both members' real voices, conduct a two-machine playtest, judge controls/visual feel, and submit to Moodle. Course-server access for hardware perf counters is unknown. These items must remain explicit in the final handoff. Nothing has been uploaded or submitted.


## User workflow update and resumed preflight

The user resumed the goal and kept the plan, phases and real subagents. Follow `PHASE_APPROVALS.md`: after phase tests pass, launch server plus two clients through WSLg, confirm windows, print a numbered manual checklist, then STOP until `APPROVED P<n>` or `FIX: ...`. No commands or edits after that manual gate. Approval is also mandatory before selecting security and before breaking IDL changes.

The resumed preflight found xvfb-run/Xvfb, ImageMagick import, tcpdump and xauth. xdotool is still absent in this Ubuntu installation (PATH and dpkg both checked); the user was informed of `sudo apt install -y xdotool`. No agent installation/sudo was attempted. The original missing-tool inventory above is historical, not the current list.

Builder is preparing P0-only play/stop/capture tools; reviewer checks lifecycle/capture safety; tester prepares the prescribed software-GL GUI smoke harness. No runtime pass is claimed. `*.pcap` is to be ignored; the user runs capture with sudo and the agent analyses it without sudo. P0 baseline traffic must be labelled plaintext, not encrypted.

P0 approval, security selection approval and breaking IDL approval have not been received. P1 remains unstarted.


## P0 verification completed; manual gate pending

xdotool is now installed: `/usr/bin/xdotool`, dpkg `1:3.20160805.1-5build1`. All prescribed P0 GUI/capture tools are present. Earlier missing-tool notes are historical.

New P0 tools: `tools/play.sh`, `tools/stop.sh`, `tools/p0_session.py`, `tools/capture_demo.sh`, and `tests/p0_gui_smoke.py`. Runtime files/captures/Python caches are ignored. `.gitignore` original byte/newline style was preserved. Baseline, third-party and IDL content was not changed.

Builder script syntax/negative-path checks passed with actual output in `TEST_REPORT.md`. Independent reviewer found no open HIGH in the new P0 tools/harness; inherited baseline findings remain open. Test run1 failed on a black capture of the obscured first window. The harness-only fix raises/repaints each window before import; run2 passed and run1 failure is retained.

The actual Xvfb run2 moved player1 49 pixels, player2 50 pixels, with both clients active 11.087 seconds. Both clients showed matching square positions and distinct Player 1/2 headings. Independent screenshots were inspected. Scope is the insecure square baseline only, not asteroid gameplay, encryption, Connected HUD, sanitizer cleanliness or two-machine networking.

Then `bash tools/play.sh` exited 0 through WSLg. Actual display was `:0`. Server pid 3779, client1 pid 3788, client2 pid 3862 were recorded by the launcher (consult current session state for authoritative PIDs). Visible client windows were 6291463 and 8388615, each 800x600 with the expected title, and mapped to the recorded client PIDs. All three executable identities were live when checked. Window screenshots are in `build/evidence/p0-desktop-d9deba26/`. The session is left open for the user. Stop using `bash tools/stop.sh` from the repository in Ubuntu.

Capture script is provided, but no capture has been run or analysed. The human runs it with sudo; the agent never does. Captures go to the invoking user's private Linux-home state directory, not Windows source storage. P0 capture is truncated/bounded plaintext reference evidence and cannot certify the future secure protocol.

Before proceeding: present the numbered manual P0 checklist and STOP. No edits, commands or P1 work until `APPROVED P0` or `FIX: ...`. Security remains a recommendation pending separate explicit approval, and no breaking IDL change is approved. Overall goal is incomplete.


## Final desktop session and lifecycle check

Duplicate play was refused with exit1. `stop.sh` stopped the three identity-checked owned processes with exit0; subsequent exact process and visible-window searches returned no rows. Relaunch exited0. The final session token is cecf1d22, with process identities [{"name": "server", "pid": 4136}, {"name": "client1", "pid": 4145}, {"name": "client2", "pid": 4219}]. Both 800x600 WSLg windows are visible on :0 and map to the recorded client PIDs. Final screenshots/geometry are in `build/evidence/p0-desktop-cecf1d22/`. Earlier desktop evidence is historical. The reopened session is left open for user manual review; live lifecycle checks now pass. No commands or edits after the numbered manual checklist until APPROVED P0 or FIX.


## User P0 approval and zero-packet capture follow-up

User: "Approved. instructions work and terminal captured this". P0 manual approval is received. They ran the capture on lo for30s and reported0 captured/received/dropped packets; the script correctly produced no evidence file. This is NOT a packet-capture pass. Primary unprivileged ss inspection found baseline DDS sockets at ephemeral ports42213,45916,52766,45495,46670,52093, with discovery7400/7401. The old7400-7600 filter can exclude game traffic; capture tooling is being corrected and independently checked offline. User alone runs sudo capture. No security or IDL choice approved, no P1 implementation started.

Capture correction complete: IPv4 RTPS magic filter, independent offline test exit0; actual traffic still awaits human retry. Updated script/runbook preserve private bounded output. Reused builder guard mistakenly stopped desktop; user informed and primary restored/confirmed server+two visible WSLg clients, session7daeed4a. P0 remains approved; security/IDL approvals pending.


User explicitly approved native DDS Security A and emphasized exact Team Lab grading compliance. Primary reread PDF tasks/deliverables/rubrics pages14/16/18/19/20 and confirmed mapped weights/requirements. Security choice is now authorized, not implemented. Final concrete P1 schema proposal updated with widths/bounds, separate v1 bindings and native-security identity/topic boundary. Actual IDL remains unchanged awaiting separate breaking-change approval. No grading requirement is waived; current project is not yet compliant with all PDF requirements.
