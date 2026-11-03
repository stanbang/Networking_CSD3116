# CODEX GOAL - build, review, and verify DDS Asteroids

Paste everything below the line into Codex (`/goal` in the CLI, run from the repo root inside WSL).
The spec lives in `docs/PROMPT.md`; `AGENTS.md` holds the rules. Read both first.

---

## GOAL

Deliver a working, secure, multiplayer 2D asteroid shooter (2-5 players) in this repo that satisfies `docs/PROMPT.md`, using Cyclone DDS + OpenGL + GLFW, with security and networking as top priority. Work as an orchestrator with three real subagents (spawn them with the subagent runtime; do not simulate them yourself):

1. **builder** - implements one phase at a time.
2. **reviewer** - independently audits the builder's work, finds issues and red flags, and fixes them (or sends them back with exact file/line evidence).
3. **tester** - independently verifies the game and security behave as intended, using commands and artefacts only.

If real subagents are unavailable in this session, say `SUBAGENTS_UNAVAILABLE`, then run the same three roles sequentially yourself, clearly labelled, with a fresh re-read of the code between roles.

## Loop (per phase P0..P6 in docs/PROMPT.md section 13)

1. Orchestrator writes the phase plan and acceptance tests into `docs/PROGRESS.md`.
2. builder implements, builds, runs, and reports real output.
3. reviewer runs the review checklist below on the actual diff and code, fixes defects it can fix safely, logs everything in `docs/REVIEW_LOG.md` (severity, file:line, fix or reason).
4. tester runs the verification suite below, writes `docs/TEST_REPORT.md` with pass/fail and raw output.
5. Any FAIL or open HIGH/CRITICAL issue returns to builder. Maximum 3 loops per phase; then stop and report the blocker to the human instead of improvising.
6. Orchestrator updates `docs/PROGRESS.md` and `docs/AI_USAGE.md`, then starts the next phase.

## Defaults (do not ask unless blocked)

Co-op gameplay; 800x600; max 5 players. Security: Option A (native DDS Security) if `libdds_security_*` plugins exist, otherwise Option B (OpenSSL application-layer AEAD + certificate auth). Record the check result and decision in `docs/PROGRESS.md`. Use `config/cyclonedds.xml` for discovery so it works on WSL2 and loopback.

## Environment limits

- You cannot `sudo`. If a package or tool is missing, stop and list the exact `apt` command for the human.
- `perf` does not work in WSL. Write the headless benchmarks in `bench/` and the exact command for the human to run on the course Linux server; use `valgrind --tool=cachegrind` and `clock_gettime` timing locally and label them as such.
- No private keys in the repo, in logs, or in reports.

## Required testability features (builder must add these)

- Client `--headless` mode (no GLFW window) with scripted/bot input (`--bot <pattern>`), so the tester can run 2-5 clients in a terminal.
- Server `--ticks N` or `--duration S` flags and a machine-readable summary on exit (players seen, joins, rejects, rate-limit hits, auth failures).
- Client flags to inject faults for negative tests: wrong certificate, no certificate, spoofed player id, replay, bit-flip, flood.
- `make test` target running unit tests (sim logic, validators) with non-zero exit on failure.

## Reviewer checklist (every phase; cite file:line)

Security: any network-derived value used as an index or size without a range check; player id trusted from message content; missing return-code checks (DDS, OpenSSL, pthread); crypto misuse (ECB, static or reused nonce, hard-coded key, `rand()` for secrets, `memcmp` on secrets, unwiped keys); plaintext game data or secrets on the wire; unauthenticated slot allocation; no replay window; unbounded sequences or loops; `printf` with non-constant format; pointers printed or logged; private keys or `.pem` in git-tracked paths.
Memory and threads: use-after-free, double free, leaks, uninitialised struct padding sent over DDS, integer overflow in size math; lock/unlock imbalance; blocking calls under a mutex; unlocked reads of shared state; spin loops; data races.
Quality: magic numbers, functions > ~50 lines, missing comments or file headers, warnings, dead code, baseline defects 1-12 from PROMPT.md section 9 still present.
Honesty: any statement in reports not backed by pasted output; any test that cannot fail.

## Tester verification suite (artefacts and exit codes only)

1. Clean clone build: `make clean && make`, `make SAN=asan`, `make SAN=tsan`, `make check-hardening` (PIE, NX, RELRO, canary shown).
2. `make test` passes.
3. Protocol soak: server + 5 headless bots for >= 2 minutes with random join/leave/rejoin; ASan/UBSan and TSan builds run clean; `valgrind --leak-check=full` shows no definite leaks or invalid accesses.
4. Gameplay assertions from the server summary: ships move, bullets destroy asteroids, scores change, lives decrease, respawn works, game-over and restart work, wrap-around works, player cap of 5 enforced, a 6th join is refused.
5. Leave handling: SIGKILL a client; its slot is freed within the lease time and a new client can take it.
6. Security negative tests (PROMPT.md 5.7): each must show the expected rejection in the server audit log, with no crash and no sanitizer report. Include a sniff test (`tcpdump`/`tshark` on loopback if permitted; otherwise state that the human must run it) proving payloads are not readable.
7. GUI smoke test: `xvfb-run -a` with `LIBGL_ALWAYS_SOFTWARE=1`, run server + 2 normal clients for 10 s, capture a screenshot of each window (`import -window root` or equivalent), inspect them, and confirm ships/asteroids/HUD are drawn and the HUD shows the connected state. If tools are missing, report what to install.
8. Final output: a **human playtest checklist** (what to look at with real eyes: smoothness, controls feel, visuals, two-machine play, Wireshark view) because automated checks cannot judge feel.

## MCP / tools

No MCP server is required. Do not add one for its own sake. If a docs-lookup MCP (for example Context7) is already configured, use it to confirm Cyclone DDS and OpenSSL API usage; otherwise rely on installed headers, `man`, and official docs. Never guess an API or XML element name.

## Done when (all with evidence in the report files)

- [ ] Phases P0-P4 complete; P5 complete or benchmarks and server-side perf commands prepared; P6 materials drafted (slide outline, video script, `docs/AI_USAGE.md`).
- [ ] Reviewer has zero open HIGH/CRITICAL findings; MEDIUM/LOW are listed with reasons.
- [ ] Tester suite items 1-7 pass; item 8 checklist delivered.
- [ ] No private key or secret appears in tracked files (`git ls-files | grep -E '\.(key|pem)$'` is empty).
- [ ] `README.md` build/run instructions verified from a clean clone.

## Final report format

Phases done, security mechanism chosen and why, test results (pass/fail table), open issues, what needs a human (server-side perf, Wireshark, two-machine test, playtest, slides, video), and exact commands to reproduce each result.
