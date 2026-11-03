# AGENTS.md - Networking_CSD3116 (DDS Asteroids)

Secure multiplayer 2D asteroid shooter for CSD3116 (DigiPen SIT). C (gnu11), Cyclone DDS, OpenGL, GLFW, OpenSSL, WSL2/Ubuntu.
The full specification is `docs/PROMPT.md`. Read it before doing anything. This file is the short contract.

## Commands
- Setup check (human runs it; you cannot sudo): `bash tools/setup_env.sh`
- Build: `make` | sanitizers: `make SAN=asan`, `make SAN=tsan` | hardening proof: `make check-hardening`
- Lecturer baseline for comparison: `make baseline`
- Certificates: `bash tools/gen_certs.sh` (output goes to an ignored folder)
- Tests: `make test` (you create this target)

## Hard rules
- Never edit `baseline/`, `third_party/`. Treat them as read-only references.
- Never commit, print, or paste private keys (`*.key`, `*.pem`). Never run `git push`, `git commit`, or change git config. The human commits with GitHub Desktop.
- Do not guess library APIs or XML config names. The installed headers (`/usr/include/dds/`), `man` pages and official docs are the source of truth. If unsure, write a tiny compile test.
- Do not claim anything works unless you ran it and saw the output. Paste real output into the report files.
- Keep the baseline naming prefixes (`b_`, `int_`, `ch_`, `arr_`, `str_`). Comment the why. Functions <= ~50 lines. Zero `-Wall -Wextra` warnings.
- Priority when time is short: security > networking correctness > threading/memory safety > measured optimisation > game polish.
- Declare AI use: append to `docs/AI_USAGE.md` after each phase (what was generated, how it was tested).

## Shared state files (update after every phase; they survive context compaction)
`docs/PROGRESS.md` (phase status, decisions, open issues), `docs/REVIEW_LOG.md`, `docs/TEST_REPORT.md`.

## Definition of done
Every item in the "Done when" list of `CODEX_GOAL.md` is checked with command output as evidence.
