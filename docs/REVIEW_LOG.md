# Latest integration status: GUI verification blocked

See the current blocker section below. Existing build/unit/headless passes are narrower than full graphical multiplayer acceptance.

# Independent review log

Purpose: P0 code and evidence review. Author: Codex reviewer (team names pending). Date: 2026-11-03.

## P0 review — blocked by dependencies

Read AGENTS.md, CODEX_GOAL.md, docs/PROMPT.md and docs/PROGRESS.md. Reviewed Makefile, tools/setup_env.sh, tools/gen_certs.sh, idl/messages.idl, .gitignore and both lecturer baseline programs. No baseline/third_party changes were made. No compilation or runtime security result is asserted.

### Open findings (all pre-existing)

| Severity | Evidence | Finding and disposition |
|---|---|---|
| HIGH | baseline/server.c:312-313,358-359; baseline/client.c:303-307 | Network player ids index fixed arrays without a range check; arbitrary invalid ids can cause out-of-bounds reads/writes. Baseline defects #1/#2; preserve reference, implement validated session-bound addressing in P1/P2. |
| HIGH | baseline/server.c:227-249,419-424; baseline/client.c:216-229; idl/messages.idl:1-27 | Unauthenticated admission and shared unaddressed join responses; no replay/AEAD/session identity. Baseline defects #2/#3/#8/#10. Must not present baseline as secure; replace in P1/P2. |
| HIGH | baseline/server.c:123-127; baseline/client.c:107-111 | Participant creation failure logs and continues. Topic/read/write construction returns and pthread mutex returns largely unchecked. Baseline defect #7; replace in P1/P4. |
| HIGH | Makefile:66-70 | check-hardening prints matches but cannot reliably fail when PIE/NX/RELRO/canary is absent; final `|| true` hides failure and BIND_NOW is not inspected. This is not proof of hardening. Builder should replace with actual failing assertions by P4. |
| MEDIUM | baseline/server.c:218,272-288,384,415-441; baseline/client.c:201,394-396 | Busy loops, mutex held across DDS write and sleep, unbounded placement loop, unlocked client identity read. Baseline defects #4/#5/#6/#12. Replace in P1/P4. Note: spec says int_num_active_players is raced, but actual server code appears to access it only in main; do not assert this particular race without evidence. |
| MEDIUM | baseline/server.c:206-218,299-366; baseline/client.c:194-201,269-284,425-428 | No sample/entity cleanup or worker join; movement rate depends on received input count; no leave/lease removal. Baseline defects #8/#9/#11. Replace in P1/P4. |
| MEDIUM | Makefile:42-44,58-63 | Baseline and future game share generated IDL bindings; replacing messages.idl in P1 will break `make baseline` unless original bindings are isolated. Ordinary multiple-output rule also permits duplicate idlc calls in parallel builds. Address when modifying protocol/build. |
| MEDIUM | tools/setup_env.sh:19 | `find ... | grep -q` under pipefail can report absent plugins if find receives SIGPIPE after a match, or if find returns an unrelated permission error. Security choice must use a complete captured file list plus explicit search status, not this pipeline alone. Static risk; not reproduced in this environment. |
| LOW | README.md:25-44; tools/setup_env.sh:5-9 | Setup script installs packages through sudo; AGENTS says human must run it. Agents must use read-only probes and supply exact human apt commands on missing dependencies. No setup script executed by reviewer. |

### Independent command evidence

Command (PowerShell launching WSL Ubuntu):

```powershell
wsl -d Ubuntu -- bash -lc 'printf "reviewer WSL check\n"; command -v gcc make pkg-config idlc openssl; pkg-config --modversion CycloneDDS glfw3 openssl; printf "ASLR="; cat /proc/sys/kernel/randomize_va_space'
```

Raw output:

```text
reviewer WSL check
/usr/bin/gcc
/usr/bin/make
/usr/bin/openssl
bash: line 1: pkg-config: command not found
ASLR=2
```

The wrapper exited 0 because its last command succeeded; the pkg-config probe demonstrably failed. This is NOT a dependency-pass result. No install attempted. Builder/tester provide complete package remediation.

Commands:

```powershell
git ls-files '*.key' '*.pem'
git diff -- baseline third_party
git diff --name-only
```

Raw output for all three was empty at review time. `git status --short` initially returned only `?? docs/PROGRESS.md`. These checks cover tracked filename extensions and unchanged reference paths; they do not prove the absence of every possible embedded secret.

## Decision

P0 remains incomplete. Existing HIGH findings are explicitly open future implementation requirements, not new support-file regressions. Dependency absence prevents build and two-client verification. No security mechanism can yet be selected from an installed Cyclone DDS plugin check because Cyclone DDS development tools are not available. Do not advance to P1 or claim definition-of-done acceptance.

## P0 resumed review after human dependency installation (2026-11-03)

The prior dependency blocker is historical. Independently reran probes; no agent performed an installation. Builder reports `make baseline` exit 0 and no source/support diff; tester owns runtime movement proof. Baseline warnings remain expected in immutable reference code (unused worker arguments and bundled font implementation warnings); do not claim warning-clean baseline. No application source exists yet.

Command:

```powershell
wsl -d Ubuntu -- bash -lc 'echo REVIEWER_RESUME; whoami; uname -a; command -v pkg-config; command -v idlc; pkg-config --modversion CycloneDDS glfw3 openssl'
```

Raw output (WSL exit 0):

```text
REVIEWER_RESUME
jordain
Linux Dain 6.6.87.2-microsoft-standard-WSL2 #1 SMP PREEMPT_DYNAMIC Thu Jun 5 18:30:46 UTC 2025 x86_64 x86_64 x86_64 GNU/Linux
/usr/bin/pkg-config
/usr/bin/idlc
0.10.4
3.3.10
3.0.13
```

Command:

```powershell
wsl -d Ubuntu -- bash -lc 'command -v xvfb-run import xdotool; find /usr /opt -name libdds_security_\* -print 2>/dev/null; cat /proc/sys/kernel/randomize_va_space; printenv DISPLAY WAYLAND_DISPLAY'
```

Raw output (WSL exit 0):

```text
/usr/bin/xvfb-run
/usr/bin/import
/usr/bin/xdotool
/usr/lib/x86_64-linux-gnu/libddsc0debian/libdds_security_crypto.so
/usr/lib/x86_64-linux-gnu/libddsc0debian/libdds_security_ac.so
/usr/lib/x86_64-linux-gnu/libddsc0debian/libdds_security_auth.so
2
:0
wayland-0
```

Security mechanism gate: prefer Option A because all three native plugin libraries exist. Library presence alone does not prove certificates, permissions, governance or wire encryption work. P2 verification remains mandatory.

`git diff --numstat` is empty. Builder confirms no P0 source/support changes. `build/gen/messages.{c,h}` and `build/release/baseline-{server,client}` now exist, but existence alone is not runtime evidence. Existing findings above remain open requirements for replacement application code; none justify modifying the immutable lecturer reference. Await tester two-client movement evidence before P0 acceptance.

Additional independent read-only inspection of built baseline server:

```powershell
wsl -d Ubuntu -- bash -lc 'cd /mnt/c/Users/jorda/Desktop/Networking_CSD3116; readelf -h build/release/baseline-server | grep Type:; readelf -lW build/release/baseline-server | grep -E "GNU_STACK|GNU_RELRO"; readelf -dW build/release/baseline-server | grep BIND_NOW; readelf -sW build/release/baseline-server | grep __stack_chk_fail'
```

Raw output:

```text
  Type:                              DYN (Position-Independent Executable file)
  GNU_STACK      0x000000 0x0000000000000000 0x0000000000000000 0x000000 0x000000 RW  0x10
  GNU_RELRO      0x002b90 0x0000000000003b90 0x0000000000003b90 0x000470 0x000470 R   0x1
 0x000000000000001e (FLAGS)              BIND_NOW
     6: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND __stack_chk_fail@GLIBC_2.4 (3)
    49: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND __stack_chk_fail@GLIBC_2.4
```

This binary has PIE, non-executable stack, RELRO plus immediate binding, and stack-canary references. This is a baseline-server observation, not verification of future game binaries or proof that the generic Makefile hardening target correctly fails. Reference diff and tracked .key/.pem filename checks again produced empty output.

### P0 GUI evidence review and acceptance

Read tester `build/p0-test/gui.sh` and `pixels.py`. Harness starts server then clients sequentially, locates windows with `--all` PID/name filtering, injects right-arrow into client 1 then down-arrow into client 2, captures both views before/after and checks processes remain alive. Process cleanup is scoped to its own saved PIDs. Visually inspected `client1-before.png` and `client2-after-p2.png`: distinct Player 1/Player 2 HUDs and both numbered square outlines are drawn. The first square moves right and the second down.

Independently reran the assertion script:

```powershell
wsl -d Ubuntu -- bash -lc 'cd /mnt/c/Users/jorda/Desktop/Networking_CSD3116; python3 build/p0-test/pixels.py'
```

Raw output (exit 0):

```text
client1: before=[(251, 130), (677, 418)], after_p1=[(270, 130), (677, 418)], after_p2=[(270, 130), (677, 438)]
PASS: both player movements visible in client1
client2: before=[(251, 130), (677, 418)], after_p1=[(270, 130), (677, 418)], after_p2=[(270, 130), (677, 438)]
PASS: both player movements visible in client2
PASS: clients agree on final square positions
```

P0 ACCEPTED for baseline build and two-client local graphical movement under Xvfb/software OpenGL, with native DDS Security plugin availability confirmed. This does not establish direct WSLg desktop interaction, LAN/multicast delivery across hosts, authenticated/encrypted game traffic, or the later Asteroids application. Existing baseline defects remain deliberately preserved; new application implementation must fix them. No new P0 code regression identified. Original IDL must be isolated before P1 changes so `make baseline` remains reproducible.

## P1 review — first implementation loop (in progress)

Read actual new common/server/client modules, v2 IDL, baseline IDL isolation, Makefile and tests. The explicitly insecure P1 intermediate is not represented as the completed secure game. Native DDS Security exact per-identity topics can support P2 certificate permissions: derive identity from the server-owned reader channel and grant clients only their own exact Join/Input/Leave writer topic names, their own Response reader and authorized Snapshot readers. No wildcard client write permissions. P2 DDS-layer rejects may never reach application counters; capture middleware diagnostics rather than inventing audit results. Discovery certificates/public identity metadata may remain visible even when gameplay payloads are encrypted.

| Severity | File:line at first review | Finding / disposition |
|---|---|---|
| HIGH | src/server/main.c:16-24 | Main samples int_now before taking mutex. Worker can record later int_seen first, causing unsigned subtraction underflow and premature lease expiry. Sent builder exact fix: sample under lock or guard now>=seen; deterministic backwards-sample regression test required. Open until verified. |
| MEDIUM | src/common/protocol.c:29-46 (initial) | Snapshot accepted seq0, implausibly large tick and _length>_maximum. Reviewer fixed metadata checks before dereferencing buffer; added boundary regressions. |
| MEDIUM | src/server/net_server.c:53-57; src/client/net_client.c:29-39 (initial) | Handshake validation was duplicated and not independently unit-tested. Reviewer added join_valid/response_valid with version/seq/nonce/status/id/session validation; builder wiring into handlers. |
| MEDIUM | src/server/net_server.c:21-51 (initial) | Join history originally vanished when slot released, allowing retired nonce to allocate again. Builder added bounded per-identity tombstones independent of slots. Active nonce retries remain idempotent and do not refresh lease. Runtime replay coverage pending. |
| MEDIUM | src/server/server.h:20-21 | Replay history accepts 64 distinct admitted nonces per identity per server process, then rejects fresh legitimate reconnects until restart. Fail-closed finite-memory tradeoff, explicitly document availability limit; improve in P2 without silently evicting protection. |
| LOW | Makefile:42-70 | Grouped IDL outputs remove parallel double-generation risk; original idl/baseline.idl isolated under build/baseline-gen and include precedence preserved. Existing baseline/third_party unchanged. |

Reviewer-owned changes in protocol.c/.h and tests/test_protocol.c: pure Join/Response validators, reserved all-zero nonce rejection, snapshot sequence/tick/backing-capacity checks, malicious boundary cases and test functions split for readability. Command `make test` (inside Ubuntu repo root) exited 0, warning-clean raw output:

```text
mkdir -p build/release
cc -std=gnu11 -O2 -g -Wall -Wextra -Wformat=2 -Wformat-security -fstack-protector-strong -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE  -Isrc -Ibuild/gen -isystem third_party   tests/test_protocol.c src/common/net.c src/common/options.c src/common/protocol.c build/gen/messages.c -o build/release/test-protocol -pie -Wl,-z,relro,-z,now  -lddsc  -lssl -lcrypto  -pthread -lm
./build/release/test-protocol
PASS protocol boundaries and CLI validation
```

Protocol edits handed back to builder, who owns network-handler integration; no concurrent edits. `git diff --check`, `git diff -- baseline third_party` produced no output. New game threading uses one DDS owner, bounded waitset sleeps, scoped loans returned, atomic client handoff and mutex-protected server state. No DDS writes/sleeps under mutex found. Strict increasing per-session sequence rejects reordered inputs (rather than accepting an out-of-order replay window); appropriate to latest input-state use, document P2. Sanitizer and long-soak acceptance remain P4.

### P1 review corrections verified

HIGH lease race CLOSED: `src/common/protocol.c:74-76` now guards `int_now >= int_seen` before subtraction. `src/server/main.c:20` uses the shared predicate. Deterministic assertions in `tests/test_protocol.c:67-69` prove a sample older than last-seen does not expire, lease-1 does not expire and exact lease does expire. This directly prevents the previously identified thread-interleaving underflow.

Handshake helpers are now wired into `src/server/net_server.c:70` and `src/client/net_client.c:31`. Snapshot validation checks metadata before buffer access. `AST_MAX_DURATION` centralizes tick/duration bounds. Server admission refactored into bounded helpers; nonce history remains independent of slots. `docs/P1_PROTOCOL.md` explicitly documents topic binding requirements for P2, insecure P1 state, latest-state per-tick consumption deferred to P3, QoS, reliable leave, history64 fail-closed reconnect limit and reproducible commands.

Independent command after these fixes:

```bash
make -j2
make test
```

Raw output (exit 0):

```text
make: Nothing to be done for 'all'.
./build/release/test-protocol
PASS protocol boundaries and CLI validation
```

`git diff --check` again produced no output. No open HIGH/CRITICAL defect in the P1 implementation after correction. The documented MEDIUM nonce-history availability cap remains; cryptographic controls/rate limiting/auditing are explicitly P2, Asteroids rendering/snapshots P3, complete sanitizer proof and fail-capable hardening script P4. Pre-existing immutable baseline vulnerabilities remain reference findings, not defects copied into the new handlers. P1 review acceptance is conditional on tester's final runtime rerun following the lease/validator fixes.

## P2 review — native DDS Security implementation (loop 1, in progress)

Reviewed actual security.c/.h, audit_log.c/.h, gen_certs.sh, options, secure participant integration, native governance/permissions generation, config/cyclonedds.xml and independent security harness/shim. Source policy enables authenticated join access control, signed default-DENY exact role/topic permissions, origin-authenticated encrypted RTPS/discovery/metadata and encrypted topic data. Client certificate subject grants allow only its own Join/Input/Leave writes and Response read plus Snapshot read; server alone writes Response/Snapshot. Normal participant creation always attaches security properties and refuses missing/unsafe material. Explicit no-cert fault mode is test-only and server policy remains secure.

### Findings and dispositions

| Severity | File/line evidence at review time | Finding / state |
|---|---|---|
| HIGH | src/server/net_server.c:86 (join_message return dds_write status) | A malicious authorized reliable Response reader can stop draining its bounded queue, causing Response TIMEOUT; treating timeout as fatal shuts down every player. Sent builder requirement to handle bounded peer backpressure as drop/counter, preserve server, and test. OPEN until fix/evidence. |
| MEDIUM | src/server/net_server.c:145-178 | Leave messages bypass rate limiter and each malformed leave emits audit text, allowing authenticated flood to bypass bounded log/work controls. Requested separate Leave bucket so an input flood does not suppress legitimate graceful Leave. OPEN until fix/evidence. |
| MEDIUM | src/common/audit_log.c:37-57 (initial) | Audit failures originally only marked final exit status while gameplay continued. Builder changed atomic failure handling: START written before worker, admission checks health, main loop stops on failure. Independent /dev/full startup denial below verified. CLOSED for identified path. |
| MEDIUM | tools/gen_certs.sh:1-114 | Full local test bundle contains all identities and both signing CA private keys. Must export minimal per-identity deployment bundles; never distribute full fixture to clients. Builder preparing export helper/docs. |
| LOW | src/common/audit_log.c:37-44 | Builder replaced monotonic-only event time with Unix wall-clock ns for accountability; monotonic clocks still govern leases. CLOSED. |
| LOW | .gitattributes:1-10 | Orchestrator created correctly named attribute file from existing misnamed gitattributes rules. Independent git check-attr confirms text=set/eol=lf for scripts/Makefile/C; no git config changed. |

Private material validation: canonical root owned by effective uid and mode0700; final path cannot be symlink; identity key regular/non-symlink/current-user/mode0600; public cert/policy cannot be group/world writable. Plugin loads from installed Ubuntu x86_64 library directory. Private Linux /tmp storage avoids drvfs777 permissions. Current-user/local administrator and policy/CA custody remain deployment trust assumptions. Same-user attacker with all test fixture keys is outside meaningful network identity isolation; minimal deployment bundles required.

Audit uses a dedicated checked mutex, EVP SHA-256 and fixed-format public events; native callback messages are classified without dumping raw payloads/addresses. Hash-chain protection requires a separately retained terminal root and record count, as documented by orchestrator. Whole-log replacement plus checkpoint replacement is not prevented. Middleware warning counters are not proof of authentication or MAC failure without correlated native operation evidence.

### Independent evidence

Command inside Ubuntu repository root:

```bash
./build/release/server --security-dir /tmp/dds-asteroids.7unxZ8xi --audit-file /dev/full --ticks 1
```

Raw output; process exit 1 (no SERVER_READY):

```text
AUDIT_ROOT hash=5beee8e05ae21972e11bd2c21d758cc3e71a4acbb196a99532400a5e3d81e88b records=0
```

Read-only permission probe:

```bash
stat -c '%a %u' /tmp/dds-asteroids.7unxZ8xi /tmp/dds-asteroids.7unxZ8xi/identities/server.key
id -u
```

Raw output:

```text
700 1000
600 1000
1000
```

No private key content read or printed. Tracked .key/.pem filename query remains empty. git diff --check produces no whitespace defect (only line-ending normalization notices after .gitattributes addition).

## P2 final checkpoint — sequential fallback review
All real agents stopped with a runtime usage-limit error. Root used the explicit goal fallback, re-reading current code for each role. No claim of independent subagent review for final changes.

- CLOSED HIGH response availability: src/common/net.c:29-31 now uses reliable KEEP_LAST(1) for idempotent Response on both endpoints. src/server/net_server.c:88 handles any TIMEOUT without terminating server. Actual non-reading peer triggered60 responses while server/honest client survived; subsequent fresh join passed.
- CLOSED implementation gap Leave limiter: src/server/net_server.c:154 selects AST_LEAVE_RATE for its third independent bucket. Dedicated malicious Leave flood runtime test remains unperformed (MEDIUM verification gap).
- CLOSED ciphertext audit observation: production native trace filter in src/common/audit_log.c:97 forwards failed decoding only, generating chained decode events; actual one-bit postcrypto transmission caused native GCM failure and server decode/gcm-final-failed diagnostics. Test-only EVP interception is explicitly separate from production audit evidence.
- CLOSED key distribution concern: tools/export_identity.sh copies only one participant key and public policy/trust material to mode0700 Linux directory; actual client1 export key stat600/current uid. Full provisioning fixture remains private and must never be distributed.
- OPEN MEDIUM, P2 acceptance blocker: src/common/audit_log.c:97 filters only failed decrypt traces; no no-cert rejection event observed. tests/p2_security.py:59 does not assert individual native rejection event deltas. A no-cert peer is denied but per-negative server-audit requirement is unverified. Do not infer no-cert logging from later wrong-CA auth_failures.
- OPEN MEDIUM: history64 availability limit remains documented; source checker exists but optional hook installation is human-owned. Existing ineffective hardening verifier queued for P4. P2 native plugin path targets Ubuntu x86_64 package layout only.

New code build/unit/security harness and secure lifecycle rerun passed; raw outputs in TEST_REPORT. baseline/third_party unchanged. README now describes the actual intermediate state and mandatory security-dir rather than implying a playable Asteroids game. P3–P6 are not complete.

## P2 resumed narrow audit fix (2026-11-04)

User authorized further implementation; parent assigned reviewer only audit_log.c/.h and tracing XML while builder owns P3 game changes. Runtime is available. Read current instructions/progress; prior three-loop checkpoint remains historical, not erased.

Official Cyclone DDS 0.10.4 source [q_ddsi_discovery.c](https://github.com/eclipse-cyclonedds/cyclonedds/blob/0.10.4/src/core/ddsi/src/q_ddsi_discovery.c#L828) defines is_secure from both the secure announcer and identity token. handle_spdp_alive logs `SPDP ST0 ... NEW` with `(secure)` only when true. A no-certificate peer bypasses certificate handshake; requiring a certificate-verification-failed warning for it was an incorrect observability assumption.

Reviewer added a strict trace classifier matching that exact function/message and missing secure marker. It emits `identity=0 event=DDS_UNAUTHENTICATED_DISCOVERY reason=nonsecure-spdp`, limited to ten records/second. It records actual nonsecure discovery without claiming cryptographic verification occurred, allocating a trusted identity, copying raw native trace data or incrementing auth_failures. The secure signed policy still disallows unauthenticated admission. Full application test must corroborate the per-case new audit record with no accepted Join and no slot growth. Native wrong-CA remains a separate authentication event. A locally denied forbidden writer cannot generate a remote server receive event and must be evidenced locally.

Independent compile (exit 0, zero warnings):

```bash
cc -std=gnu11 -Wall -Wextra -Isrc -Ibuild/gen build/review/audit_probe.c src/common/security.c src/common/audit_log.c -o build/review/audit-probe -lddsc -lcrypto -pthread
```

Independent native integration probe `python3 build/review/audit_probe.py` uses separate finite secure/insecure participant processes and the real loopback configuration; no application endpoints or game binaries were changed. Exit 0, raw output:

```text
insecure_exit= 0
server_exit= 0
PROBE_READY
DDS_DIAGNOSTIC reason=unauthenticated-discovery function=handle_spdp_alive cause=nonsecure-spdp
AUDIT_ROOT hash=3d415dfaf2fb3b05be78fdd8299467eacbbd777e41cbb6aa983b6d700446a4b8 records=1
seq=1 ns=1793725069788296596 identity=0 event=DDS_UNAUTHENTICATED_DISCOVERY reason=nonsecure-spdp previous=0000000000000000000000000000000000000000000000000000000000000000 hash=3d415dfaf2fb3b05be78fdd8299467eacbbd777e41cbb6aa983b6d700446a4b8
PASS actual native no-cert discovery audit
```

Artifacts: build/review/audit_probe.c, audit_probe.py, probe-server.log, no-cert-audit.log. No private key contents read or output. Tester owns strict per-negative application audit-delta assertions and will run them against the coherent P3 build; this standalone probe is not full game/security suite acceptance.

Additional independent check: `python3 tests/verify_audit.py build/review/no-cert-audit.log --expected-root 3d415dfaf2fb3b05be78fdd8299467eacbbd777e41cbb6aa983b6d700446a4b8 --expected-records 1` exited 0:

```text
PASS audit chain records=1 root=3d415dfaf2fb3b05be78fdd8299467eacbbd777e41cbb6aa983b6d700446a4b8
```

Reviewed tools/play_local.sh: bounded server readiness, secure private Linux bundle generation, cached path only in ignored build, own active-job cleanup with TERM/KILL bound, and window-close session handling are appropriate. Initial LOW grant-expiry cache gap was sent to parent and corrected: verify public signed permission policy against permissions CA and require not_after beyond session end. bash -n passes; actual launcher/runtime proof awaits coherent P3 build. Preserve native tracing in any custom LAN CYCLONEDDS_URI, since precise audit discovery/decrypt observation depends on it. Reference-tree diff and tracked .key/.pem checks still return empty.

## P3 actual implementation review (2026-11-04)

Reviewed sim.c/.h, frame.c/.h, updated v2 IDL, protocol validators, server main/network, client ring/bot/network/render and local launcher. No open HIGH/CRITICAL found in reviewed P3 code. Native security permissions already reserve only server Snapshot_0 publication; client reads do not confer authority.

| Severity | Location | Review result |
|---|---|---|
| MEDIUM, fixed | src/client/render.c hud/render_run | Assigned id alone previously displayed CONNECTED without any authoritative game snapshot, and rejected GUI exited too fast to read. Builder now requires own ship in a fresh frame, shows WAITING FOR GAME STATE / CONNECTION LOST appropriately, and keeps rejected GUI visible until close/deadline. Headless rejection remains finite. |
| MEDIUM, prevented | src/server/main.c game_step | Reviewer flagged same-tick slot reuse risk before integration. Builder tracks arr_sim_session; generation change disconnects/reset/connects ship and clears owned bullets, avoiding old identity score/life inheritance even if b_active stays true. |
| MEDIUM, test gap fixed | tests/test_protocol.c entity_cases | Ship-only validator fixtures omitted bullet/asteroid malformed sequences and fields. Reviewer added oversized lengths, backing-capacity mismatch, NULL buffers, duplicate/zero ids, invalid/missing owner slot, out-of-arena coordinates, invalid size and ship health/timer/speed cases. |
| LOW / P5 | src/client/render.c hud | Glyph geometry is cached by text contents, but dynamic HUD snprintf still occurs each render frame. Measure/optimize only in P5; no claimed optimization result. |

Safety findings: fixed-size pools bound all entity loops; physics consumes one input state per 60Hz tick, independent of packet count. Inertia/friction, toroidal movement/collision, bounded splitting and cooldown are pure deterministic logic. Network input remains player-session validated before indexing. Server frame conversion initializes every serialized field and uses bounded integer hundredth-pixel coordinates. Snapshot validation checks all sequence metadata before dereferencing, every ship id/health/timer/speed, entity ids/owner/size/positions and duplicate ids before bots/renderer use them. Native ciphertext never contains raw C pointers.

DDS loans are validated then deeply copied into pointer-free Frame before return. Frame copies cross the client mutex, no loan pointer survives in the ring/renderer. Renderer matches object ids, handles toroidal angle/position interpolation, and draws only server positions. Input timestamp is an untrusted echo used solely for guarded local RTT estimation, not authorization/lease/security time. Client/worker shared flags and scalar input use atomics. Network writes occur outside game-state mutexes.

Reviewer compiled the strengthened validator test to a separate binary, preserving tester's active server/client binaries:

```bash
cc -std=gnu11 -O2 -Wall -Wextra -Isrc -Ibuild/gen tests/test_protocol.c src/common/protocol.c src/common/options.c -o build/review/test-protocol -lcrypto -lm -pthread
./build/review/test-protocol
```

Exit 0, zero compiler warnings, raw output:

```text
PASS malformed bullet/asteroid/health snapshot boundaries
PASS protocol boundaries and CLI validation
```

Independently viewed actual `build/p3-gui/client1-10s.png` and `client2-after-input.png`: distinct connected secure P1/P2 HUDs, line ships, varied asteroid outlines, bullets, lives/score and controls are visible. Read live five-bot summary:

```text
SERVER_SUMMARY joins=5 rejects=0 leaves=5 timeouts=0 invalid=0 stale=0 peak=5 inputs=1697 rate_hits=0 auth_failures=0 middleware_events=0 moves=9831 shots=831 destroyed=223 splits=100 hits=7 respawns=6 gameovers=0 restarts=0 wraps=30 snapshots=704 points=16280 ticks=2134
```

This proves measured gameplay effects for that run; it does not prove live game-over/restart (both zero). Independent pure simulation fixtures exercise real death/respawn/game-over/restart transitions, while full end-to-end game-over/restart summary coverage remains to be obtained for final suite acceptance. Sanitizers, two-minute/final soak, Valgrind, external sniff, multi-machine play and human feel remain separately unverified; no complete GOAL claim made here. Await final P2 per-case security and P3 flood-speed/launcher runtime evidence from tester/parent.

### P2/P3 resumed acceptance (2026-11-04)

Final independent tester security run exited0. Reviewed actual per-case audit-delta test code: no-cert records a new unknown-identity nonsecure discovery observation with no identity6 JOIN; wrong-CA separately records AUTH_WARNING; locally forbidden authoritative writer is rejected before publication and evidenced locally, not misrepresented as a remote server audit. Input/Join/Leave flood each requires a new corresponding RATE record and continuing honest snapshots; sampled velocity and authoritative move/tick bounds are checked. After one-bit postcrypto corruption, actual native GCM failure and production chained decode events are observed. Snapshot topic is governed by the same server-only encrypted signed policy; external network sniff remains the documented human check, not claimed from process-local input mutation evidence.

Actual final security summary and trusted root:

```text
SERVER_SUMMARY joins=7 rejects=45 leaves=6 timeouts=1 invalid=14 stale=0 peak=2 inputs=592 rate_hits=6570 auth_failures=6 middleware_events=17 moves=3207 shots=270 destroyed=44 splits=23 hits=0 respawns=0 gameovers=0 restarts=0 wraps=0 snapshots=847 points=2980 ticks=2584
AUDIT_ROOT hash=ae80796d4a87e3cc0118fb217fcded64b7622cc733c11bcc32be2c264c34afcd records=136
NATIVE_GCM_SUMMARY failures=10
```

Reviewer independently ran the audit verifier against that separately retained root/count, exit0:

```text
PASS audit chain records=136 root=ae80796d4a87e3cc0118fb217fcded64b7622cc733c11bcc32be2c264c34afcd
```

Previous HIGH reliable-response denial risk is CLOSED by latest-only reliable Response QoS plus nonfatal timeout handling and tester's real slow-reader case exceeding the prior32-response queue bound without terminating healthy players. Previous unthrottled Leave MEDIUM is CLOSED by dedicated5/s bucket and observed Leave RATE test. An abusive identity can exhaust its own Leave bucket and wait for the3s lease; harness now accounts for that documented self-limitation. Minimal per-identity export removes CA/server keys from client deployment bundles. Existing MEDIUM history64 lifetime cap remains explicit.

Reviewed controlled server-only `--test-scenario gameover` fixture: false by default, not network-triggerable, executes once after>=2 real sessions and180ticks, changes only lives/invulnerability/velocities/nearby collision setup. No evidence counters or gameover/restart flags are forged. Actual sim_tick causes death/gameover and secured hunter client input R causes restart. Real logs:

```text
SERVER_TEST_SCENARIO name=gameover tick=181
GAME_EVENT gameover tick=182
GAME_EVENT restart tick=192
SERVER_SUMMARY joins=2 rejects=0 leaves=2 timeouts=0 invalid=0 stale=0 peak=2 inputs=186 rate_hits=0 auth_failures=0 middleware_events=4 moves=1085 shots=93 destroyed=29 splits=16 hits=2 respawns=0 gameovers=1 restarts=1 wraps=3 snapshots=210 points=1920 ticks=632
```

Both actual clients log gameover1 at tick184 and gameover0 at194, with lives0 before restart and lives3/score0 afterwards. Reviewer independently verified that run's six-record audit, exit0:

```text
PASS audit chain records=6 root=e28703a29f865ccb8b55b813651eaf0bc348d74a7f4d476e69e95de7433cb483
```

This closes the previously unobserved real-server game-over/restart transition requirement using an explicitly labeled controlled collision setup, separate from normal five-bot gameplay. Final GUI survived10.5s and shows connected fresh authoritative state after HUD corrections. Parent verified launcher real runtime exit0 and cleanup. P2/P3 review ACCEPTED with zero open HIGH/CRITICAL findings in the new implementation. External sniff, cross-machine play, feel, full sanitizer/Valgrind/soak/hardening/performance/submission acceptance remain distinct P4+ work; no full-GOAL completion implied.

## P4 review — hardening and runtime prerequisites (2026-11-04)

Read tools/check_hardening.sh and Makefile changes. Original fail-open hardening target is now CLOSED: assertions require actual ELF DYN plus PIE flag, exactly one non-executable GNU_STACK, GNU_RELRO plus immediate binding and an undefined __stack_chk_fail dependency. Each readelf status is checked; aggregate failures cannot be hidden by a later good executable. Builder refined canary parsing to stable symbol-table columns, handling stripped dynamic symbols with version indexes and rejecting a mere local dummy symbol name. Release protections remain enabled; sanitizer outputs are separate, unknown SAN values fail rather than silently producing release.

Reviewer compiled genuine independently weakened ELF fixtures using build/review/elf_probe.c and ran build/review/elf_check.sh. Exit0 from the test harness means every expected failure occurred:

```text
PASS PIE NX RELRO BIND_NOW CANARY references=2
EXPECTED_REJECT no-pie exit=1: FAIL build/review/elf/no-pie PIE: requires DYN and dynamic PIE flag
EXPECTED_REJECT executable-stack exit=1: FAIL build/review/elf/executable-stack NX: requires one non-executable GNU_STACK
EXPECTED_REJECT no-relro exit=1: FAIL build/review/elf/no-relro RELRO: GNU_RELRO segment absent
EXPECTED_REJECT lazy-binding exit=1: FAIL build/review/elf/lazy-binding full RELRO: BIND_NOW/NOW absent
EXPECTED_REJECT no-canary exit=1: FAIL build/review/elf/no-canary canary: no __stack_chk_fail reference
EXPECTED_REJECT missing exit=1: FAIL build/review/elf/missing missing/unreadable
PASS PIE NX RELRO BIND_NOW CANARY references=2
PASS aggregate bad-first/good-last still exits1
PASS independent ELF checker negative fixtures
```

After stripping an independent good fixture, the refined checker still passes its real dynamic canary import:

```text
PASS PIE NX RELRO BIND_NOW CANARY references=1
```

Static lifetime review reconfirmed sole DDS ownership, bounded loans always returned, pointer-free handoff under paired locks, no network I/O/sleep under simulation mutex, session generation reset, and join-before-summary/destruction. Installed `/usr/include/dds/ddsrt/log.h` explicitly documents synchronous sink registration/unregistration: old callback/userdata no longer referenced upon return. Server unregisters both sinks before destroying audit storage; client unregisters trace after participant deletion.

Builder identified one MEDIUM diagnostic-rate race: callback sampled time before another callback advanced the atomic last timestamp; unsigned elapsed subtraction could wrap and CAS move the throttle clock backward. Reviewer fixed both middleware_log and unauthenticated_discovery with now<previous guards before subtraction/CAS. No player admission/state corruption was involved. Syntax-only compile exited0 warning-free; active ASan runtime binaries were deliberately not replaced. Next coherent build must include these guards.

### TSan environment diagnosis — no false pass

Independent minimal probe contains one pthread writing an int followed by pthread_join before main reads it, with no DDS/OpenGL/OpenSSL. Compiled warning-free with `-fsanitize=thread`. Default PIE execution exited66:

```text
FATAL: ThreadSanitizer: unexpected memory mapping 0x60b39e880000-0x60b39e881000
```

A diagnostic-only non-PIE copy was nondeterministic: exits0,66,66,0. This is not a reliable solution and no game compiler protection was weakened. Parent explicitly authorized per-process setarch investigation, without sudo or global ASLR changes. `setarch x86_64 -R ./build/review/tsan-probe` then exited0 on four consecutive executions. `/proc/sys/kernel/randomize_va_space` remains2. This permits a separately labeled per-process reduced-ASLR TSan test environment; actual game TSan session evidence remains tester work. A minimal probe pass is not a race-free-game claim.

Artifacts: build/review/sanitizer_probe.c, tsan-probe, tsan-probe-nopie; build/review/elf_check.sh and genuine positive/negative ELF files. No global sysctl, sudo, private key output or immutable-reference edits occurred. Await real ASan/UBSan/TSan/Valgrind/session output before P4 acceptance.

## P4 sequential fallback final checkpoint

All real agents hit quota. Root re-read final sources, reviewed actual test harnesses/logs and executed remaining checks sequentially under the goal fallback; no independent-agent pass is invented.

- CLOSED MEDIUM callback throttle underflow race: both auditcallback paths now reject stale sampled time before subtraction/CAS. Coherent release/ASan/TSan builds are current; final native ASan/UBSan audit probe produced genuine unauthenticated-discovery event and no sanitizer report, both exits0.
- CLOSED hardening-verifier defect: release,ASan,TSan ELF checks pass; independent real weakened fixtures fail as required. Fresh final source-copy all/test/hardening exits0.
- PASS actual ASan/UBSan five-player120s seeded churn,16replacements including3SIGKILL, no process diagnostics, final audit verified. Explicitly not ten-minute soak.
- OPEN HIGH verification/dependency finding: unsuppressed actual-game TSan session reports DDS internal recvmsg/memcpy and mutex-destroy/unlock races and client exit66. System DDS/plugin are uninstrumented; root does not conclude false-positive or source race-free. Need instrumented matching dependency investigation.
- OPEN HIGH verification/dependency finding: Valgrind clients report uninitialised sendmsg bytes with origin inside libcrypto/nativeDDScrypto path. All six processes free all heap blocks, but errors prevent Memcheck acceptance. Do not suppress or dismiss as padding without proof.
- ENVIRONMENT BLOCKER: cmake absent in Ubuntu (which exit1). Per goal install boundary, human must install cmake before isolated dependency build/investigation; no sudo/install/globalASLR changes by agent.
- Existing MEDIUM reconnect-history64 bound, LOW HUD overlap and untested LAN/human feel remain documented. New gameplay code had no open HIGH/CRITICAL at P3 acceptance; these later dependency findings mean overall project verification is not complete.

Exact failed/passed commands and raw excerpts are in TEST_REPORT. P0–P3 playable foundation/game complete; P4 partial; P5/P6 not started. Do not conflate game implementation with full security/memory certification.


## Local main integration,4November2026

See INTEGRATION.md. Source2cad2e1 fast-forwarded after preserving localP0 history/bytes; nativeA/IDL/P1 integration authorized by user. Independent agents compiled/reviewed/ran initial failedP1 test, then usage limits caused SUBAGENTS_UNAVAILABLE sequential fallback. Fallback fixed audit overwriting with a real preservation regression and strengthened fresh-output/join-aware fault tests; actual diff reread before checks. Units passed; networkingrun3 passed but original shutdownhang stays unresolved. No sudo/commit/push, no privatekeys inrepo. P4 HIGHs and P5/P6 work remain.


## Current blocker: real secure GUI acceptance failed

Actual `xvfb-run ... tests/p3_gui.py --security-dir <persistent Linux bundle> --output-dir build/evidence/integration-gui-run1` exited1. Both windows rendered, but the server summary had joins2, peak1, timeouts2, inputs5, moves0, shots0, snapshots40, ticks1500. The first lease expired before the second player joined. Both final PNGs show WAITING FOR GAME STATE rather than a current connected two-player world. Root inspected both actual images. This contradicts accepting the merged game as working locally on this host, regardless of contributor historical reports. It is not cleared by the headless run3 pass.

Evidence: build/evidence/integration-gui-output.txt, integration-gui-exit.txt, integration-gui-run1/*.log and client1-10s.png/client2-10s.png. Cleanup completed; no successful manual GUI session is currently left open. Do not fabricate a manual-phase pass or open the game as if all own checks passed.

The current bounded integration attempt cycle is stopped at the original maximum-three-attempt rule. The middle missing-/tmp-bundle prerequisite failure is disclosed, not silently erased to extend the limit. Request a human decision to authorize a new targeted GUI/network starvation/hang investigation cycle. No fourth network attempt or second GUI attempt has been run. A diagnosis is not established: do not label native dependency, GPU, clock, or the harness as the cause without stronger evidence.

The merged implementation and all local work remain preserved. P1 integration acceptance is INCOMPLETE despite passing headless functionality. Native-security and P3 source exist; P2/P3 have not been newly accepted here. P4 OPEN HIGH TSan/Valgrind findings, full soak, external capture, two-machine verification, P5 measurements and P6 deliverables remain required by the original scope/PDF checklist.


## Paused teammate checkpoint
User explicitly paused goal dueusage and authorized handover+commit/push. HANDOVER.md is current; selected actual nonsecret output copied under docs/evidence/2026-11-04-handover/ because build/ is ignored. Native ACK diagnostic measured requested1e9ns elapsed1000082330ns result-10; this is not a waitset overshoot. Latest active GUI has timeouts0/invalid0/freshconnected state, but overall network and GUI remain FAIL during ACK shutdown. No game tests/repairs ran afterpause, no private material included, no completion claim.
