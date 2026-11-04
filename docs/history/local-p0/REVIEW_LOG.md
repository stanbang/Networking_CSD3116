# Independent review log

Purpose: preserve actual P0 findings. CSD3116 team with Codex assistance, 4 November 2026 (Singapore).

Real reviewer subagent performed a fresh read of the code, Makefile, IDL, configuration inventory and certificate script. No source edits were made. All findings below are inherited starter defects or missing implementation, not regressions introduced in this session. Baseline files must remain read-only; fixes belong in new production modules in P1-P4.

## Raw scope evidence

```text
$ git diff --ignore-space-at-eol --stat -- baseline/server.c baseline/client.c
[no output]
$ git ls-files -- '*.key' '*.pem'
[no output]
$ find config -maxdepth 1 -type f
config/.gitkeep
```

## Open findings

| ID | Severity | File:line evidence | Required disposition |
|---|---|---|---|
| R01 | CRITICAL | `baseline/server.c:304,312-313,358-359`; `baseline/client.c:300,303-307` | Unchecked network IDs index fixed arrays. Validate IDs before use in new code; test zero, negative and above-cap values. |
| R02 | HIGH | `baseline/server.c:116-121,227-234,304,358-359`; `idl/messages.idl:3-25` | Unauthenticated allocation and message-supplied identity, plaintext traffic and no replay protection. Implement native DDS security and authenticated session binding before allocation. |
| R03 | HIGH | `idl/messages.idl:8-12`; `baseline/client.c:216-230` | Every waiting client can accept a shared join response. Echo random client nonce and validate assignment. |
| R04 | HIGH | `baseline/server.c:299-359` | Input arrival rate directly controls movement. Apply one state per simulation tick, enforce per-session rate budget. |
| R05 | HIGH | `baseline/server.c:272-288` | DDS writes and sleeps occur while holding mutex. Copy under lock; perform I/O after release. |
| R06 | HIGH | `baseline/server.c:415-455`, especially `429` | Unbounded placement attempts under mutex. Bound retries and provide deterministic fallback. |
| R07 | HIGH | `baseline/client.c:229,394-396` | Player ID is written under lock and read without it by render thread. Use consistent locking/atomics. |
| R08 | HIGH | `baseline/server.c:123-127,133-207`; `baseline/client.c:107-111,117-196` | Failed participant creation only logs and continues. Entity and sample allocation results unchecked. Fail safely and clean up. |
| R09 | HIGH | `baseline/server.c:218,384`; `baseline/client.c:201` | Polling loops lack an idle waitset/condition-variable path. Replace in new implementation and measure CPU. |
| R10 | HIGH | `baseline/server.c:422-424`; `idl/messages.idl:3-25` | No leave or lease expiry; crashed clients retain slots. Add authenticated leave and timeout. |
| R11 | HIGH | `Makefile:66-70` | Hardening target prints properties and masks missing canary with `|| true`. Enforce PIE, NX, RELRO, BIND_NOW and canary failures; record ASLR separately. |
| R12 | HIGH | `tools/gen_certs.sh:29-35`; `config/.gitkeep` | Native DDS security is not configured; signed governance and role permissions absent. Installed plugins are not an active security mechanism. |
| R13 | MEDIUM | `baseline/server.c:206-213,218`; `baseline/client.c:194-201,425-428` | No sample/entity cleanup, worker stop/join or finite lifetime; outgoing structs not zero-initialized. New modules need ownership and shutdown paths. |
| R14 | MEDIUM | `tools/gen_certs.sh:7-10,15,22,29-32` | Regeneration overwrites trust roots and identity keys. Document safe regeneration/renewal, compromise handling and effective file permissions on NTFS. |
| R15 | MEDIUM | `.gitignore:15-22`; tracked-file inventory | Ignore rules exist but no pre-commit secret guard. Add guard without changing git config. |

Status: every finding OPEN. No false zero-HIGH/CRITICAL claim is permitted. All 12 baseline defect categories remain represented in the starter. No hash-chain audit log, production security tests or secure asteroid implementation exists.

Accuracy note: the review did not find competing worker accesses to the server's `int_num_active_players`. Its unlocked read at `baseline/server.c:406` alone does not prove a data race. Do not repeat the master prompt's example as a verified race without actual competing accesses. The client player-ID race is independently supported above.

No private-key or PEM paths were returned by the tracked-path scan. This is narrower than proving absence of every secret in all file contents/history. No keys were generated in this session.


## New P0 tooling review and runtime evidence

The real independent reviewer inspected the final launcher/helper, capture script and GUI harness. No open HIGH issue was identified in these new P0 tools. This does not close any inherited baseline finding above.

Review-driven fixes: refuse live ownership mismatches and any existing local baseline process including deleted/rebuilt executable paths; clean up a failed direct child safely even if pidfd open fails; identity-checked pidfd stop; shared lifecycle lock; reject uncontrolled DDS config; preserve earlier evidence directories; verify private Linux-home capture ownership/modes; explicitly drop capture privileges to the invoking user; bound capture duration, packet count and snap length. Synthetic deleted-executable/evidence-preservation checks and actual safe-guard output were independently read.

Actual GUI run1 failure (black hidden-window capture) was retained. Reviewer independently viewed the initial valid PNG and black PNG, then approved raising/repainting only the target window before capture. Actual run2 was independently reviewed against summary and before/final PNGs: distinct player headings, matching positions, 49/50px movement, 11.087s two-client lifetime. No baseline edits were needed.

Known MEDIUM limitation: capture default-domain port filter can include unrelated DDS traffic on the chosen interface and truncates each packet at256 bytes. The script warns accurately; it is P0 reference tooling and must be revisited for later protocol/ciphertext evidence. No packet capture or root capture path has been executed. The safe stop logic was reviewed and the primary subsequently tested live duplicate refusal, identity-checked three-process stop, absence of remaining baseline windows/processes, and relaunch. Actual output is in TEST_REPORT; the reopened session is intentionally left open.

Capture-filter correction review: fixed-range assumption contradicted by observed ephemeral DDS sockets. Replacement `ip and udp and udp[8:4] = 0x52545053` checked against installed Cyclone header and libpcap manual; bounded/private capture unchanged. Tester offline positives/negatives passed, including explicit magic-prefix false-positive limit. IPv6/reassembly/session exclusivity and encryption are not established. Builder reused guard wrongly stopped desktop; incident disclosed and managed session restored by primary.
