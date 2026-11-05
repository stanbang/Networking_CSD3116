# Preflight before two-PC testing — 5 November 2026

The local working tree passed a fresh `-Werror` build, unit tests and hardening under Ubuntu/WSL2. Account integration, strict two-window gameplay, P1 lifecycle and P2 native security each exited 0. The runtime checks used a separate DDS UDP port base with the same domain 42 and signed policies to avoid interfering with the user's local session. The current account build is still local.

```text
PASS two accounts played and server saved an earned best of 520
PASS persisted leaderboard, token rotation and replay rejection, credential binding, logout and forget revocation
PASS secure two-GUI lifecycle, real key input and >=10s rendering; inspect screenshots
SERVER_SUMMARY joins=13 rejects=1 leaves=12 timeouts=1 invalid=4 stale=1 peak=5 inputs=731 rate_hits=0 auth_failures=0 middleware_events=20 moves=4597 shots=388 destroyed=61 splits=27 hits=1 respawns=1 gameovers=0 restarts=0 wraps=0 snapshots=463 points=4480 ticks=1424
PASS P1 networking lifecycle and malformed-message counters
PASS actual ciphertext send: byte=52 packet_bytes=344
PASS receiver OpenSSL GCM authentication failure observed
PASS audit chain records=134 root=1350aff7d1ea0ef1d60b96e324322947c64111dfae1916f7816b99b44e6f9b4b
PASS non-loopback DDS unicast and three separately exported role bundles on one WSL host
```

The last probe selected `eth0` and a local unicast peer. The server and both clients each used a minimal exported credential bundle. Both clients registered, received snapshots and left cleanly. The non-loopback interface and role bundles worked on one WSL host. Cross-machine routing, VPN reachability and the second PC's build still need testing. No VPN or firewall settings were changed. Raw outputs, scratch configuration and inspected GUI screenshots are in `build/evidence/preflight-20261105/`.

# J1–J4 account verification — 5 November 2026

Ubuntu on Jordain's Windows machine, SQLite 3.45.1, Cyclone DDS 0.10.4. The binaries were built from the local working tree on `480227c`, including the preserved local integration changes. The fresh build used new generated-IDL and output directories with `-Werror`. The native tests ran sequentially on domain 42 with a current private Linux-home bundle.

| Check | Result and retained evidence |
|---|---|
| Fresh build, units and hardening | Exit 0. Both IDLs regenerated. Both binaries report `PASS PIE NX RELRO BIND_NOW CANARY references=2`. Account validators passed length, ASCII, role, sequence and leaderboard boundaries. |
| SQLite unit and storage review | Exit 0 with private ownership and file modes, exclusive lock, hashed token, expiry, revoked and rotated token rejection, wrong identity, RNG rollback, lower/duplicate score idempotence, provenance and refusal to overwrite unsafe or corrupt files. Separate strict UBSan fixture also exited 0. |
| Live accounts and restart | Exit 0, `build/evidence/accounts-live-04/`. A native-certified peer could not join without an account or publish an account response. Forged account session, unknown account, rapid retry, duplicate active account, wrong password and wrong certificate identity were denied. The server retained an earned best of 270 and returned it from the restarted process. Resume across client restart rotated the token. Old-token replay was denied. Logout and Forget removed the profile and revoked the token. |
| P1 lifecycle | Exit 0, `build/evidence/accounts-p1-02/`. `joins=13 rejects=1 leaves=12 timeouts=1 invalid=4 stale=1 peak=5`. Fresh explicit Leaves, replacement, the deliberately killed client's lease and protocol faults all matched expectations. Audit verified 58 records with root `51995368ab17f004324ae60a63892d28959618d98b7a838beba425dee7dc7333`. |
| Native P2 negative security | Exit 0, `build/evidence/accounts-p2-01/`. No-cert, wrong-CA and rogue-server refused. Slow reader, three rate-limited gameplay floods and real post-encryption tamper checks passed. Receiver OpenSSL GCM failure was observed. Audit verified 127 records and detected both content modification and truncation. |
| Strict two-window GUI | Exit 0, `build/evidence/accounts-gui-02/`. Both players typed registration details, received fresh game state and moved/fired with real X input. Two final PNGs show connected players, ships and asteroids. [Masked login](evidence/2026-11-05-accounts/client1-masked-login.png), [P1 after 10 seconds](evidence/2026-11-05-accounts/client1-10s.png), [P2 after 10 seconds](evidence/2026-11-05-accounts/client2-10s.png). |
| Live game-over and restart | Exit 0, `build/evidence/accounts-gameover-01/`. Controlled collision produced game-over at tick 182, and ordinary encrypted client input restarted at tick 189. Both clients left gracefully. |
| Default launcher and private data guard | Exit 0 for `bash tools/play_local.sh --headless --duration 8` with newly provisioned Linux-home credentials and two admitted clients. `tools/check_secrets.sh` passed. |
| Two-minute launcher run | Exit 0, `build/local-session.wViKo70j/`. `joins=2 timeouts=0 invalid=0 moves=14209 destroyed=263 points=19280 ticks=7200`. Both clients received snapshots through tick 7195 or later and exited normally. The server reached its own deadline first and committed both scores at shutdown. |
| Graphical launcher entry | Exit 0 under Xvfb, `build/local-session.0wVekLii/`. Both windows reported `CLIENT_LOGIN_READY` and exited normally at the session limit. The separate strict GUI test exercised registration and gameplay. |

Selected actual test output:

```text
PASS account length, ASCII, role, sequence and leaderboard boundaries
PASS exclusive store lock and private file modes
PASS score max and leaderboard persist across database reopen
PASS verifier storage, hashed token and original score provenance
PASS token expiry, binding, rotation, rollback and revocation
PASS unsafe database paths refused and existing bytes preserved
PASS certificate alone cannot join or publish an account response
PASS forged account session, unknown account and rapid credential retry denied
PASS two accounts played and server saved an earned best of 270
PASS persisted leaderboard, token rotation and replay rejection, credential binding, logout and forget revocation
PASS P1 networking lifecycle and malformed-message counters
PASS actual ciphertext send: byte=52 packet_bytes=344
PASS receiver OpenSSL GCM authentication failure observed
PASS P2 harness assertions; native rejection/rate details require report evidence
LIVE_STATE client=1 id=1 samples=12 first_tick=131 last_tick=797
LIVE_STATE client=2 id=2 samples=12 first_tick=151 last_tick=815
SERVER_SUMMARY joins=2 rejects=0 leaves=2 timeouts=0 invalid=0 stale=0 peak=2 inputs=225 rate_hits=0 auth_failures=0 middleware_events=5 moves=793 shots=12 destroyed=3 splits=3 hits=2 respawns=2 gameovers=0 restarts=0 wraps=0 snapshots=272 points=90 ticks=826
PASS secure two-GUI lifecycle, real key input and >=10s rendering; inspect screenshots
GAME_EVENT gameover tick=182
GAME_EVENT restart tick=189
PASS controlled encrypted collision/game-over/ordinary-input restart
```

The first GUI run exited early because its temporary certificate directory had disappeared between WSL sessions. The persistent Linux-home bundle resolved that prerequisite. An intermediate new rate-limit probe omitted the fresh HELLO required after a failed login. The corrected probe completed on the same game build. Evidence folders `accounts-gui-01` and `accounts-live-03` retain the failed runs.

Reproduce inside Ubuntu with a new signed bundle and new output names:

```bash
make -j2 all test build/release/account-join-probe check-hardening
python3 -B tests/account_integration.py --security-dir "$BUNDLE" --output-dir build/accounts-live-check-01
python3 -B tests/p1_network.py --security-dir "$BUNDLE" --output-dir build/accounts-p1-check-01
cc -shared -fPIC -Wall -Wextra tests/rtps_fault.c -o build/rtps_fault.so -ldl -pthread -lcrypto
python3 -B tests/p2_security.py --security-dir "$BUNDLE" --output-dir build/accounts-p2-check-01
xvfb-run -a -s '-screen 0 1620x650x24' python3 -B tests/p3_gui.py --security-dir "$BUNDLE" --output-dir build/accounts-gui-check-01
python3 -B tests/p3_gameover.py --security-dir "$BUNDLE" --output-dir build/accounts-gameover-check-01
bash tools/play_local.sh --headless --duration 120
```

Manual acceptance, external capture, two-machine play, dedicated global account-channel saturation, injected disk-full recovery and fresh published-checkout reproduction remain open. Native TSan/Valgrind dependency findings are tracked separately.

---

# J0 proposal feasibility — 5 November 2026

Scope: account-feature design only. No production login, C storage, remembered profile, new topic permission or game-schema implementation is tested/claimed.

| Check | Actual result |
|---|---|
| Current source inspection | Account login/storage/leaderboard/profile modules absent; scores are volatile |
| SQLite C dependency | Header/pkg-config support missing; installation requires approval and human action |
| Draft `accounts_v1.idl` | Installed Cyclone idlc exits 0 in scratch output |
| Proposed `idl/accounts.idl` patch | `git apply --check` exits 0; production file remains absent and patch unapplied |
| Draft SQL via Python SQLite 3.45.1 | In-memory schema, one-account/identity, capacity six, username constraints, max/lower/duplicate score behavior, account isolation, rollback, token binding/expiry/compare-and-swap constraints pass |
| OpenSSL scrypt feasibility | Warning-strict compile and run exit 0; N=131072/r=8/p=1, maxmem167772160, three derivations428315367/408776511/336227400ns, peakRSS136600KiB; matching and changed-input checks pass |

No secret values printed, no real accounts/database/profile created, no new package installed, no native investigation/network session run. SQL smoke checks use Python's existing SQLite runtime, not the missing C development package. Selected outputs: `docs/evidence/2026-11-05-accounts-j0/`; scratch sources/complete outputs: `build/account-j0/`. Independent design review corrections cover pre-Join liveness, flow replay/cancellation, secret-cache invalidation, score-mailbox versioning and same-certificate trust. Implementation/runtime acceptance is pending approval.

---

# Pull integration verification — 5 November 2026

Tested `480227c` plus restored local work, after resolving the five upstream/stash overlaps. This is verification of the combined working tree, not a claim that uncommitted work is published or all project phases are complete.

| Check | Actual result |
|---|---|
| `make -j2 all test check-hardening` | Exit 0; protocol, simulation and event-queue units plus both executable hardening checks pass |
| Strict timeout fixture compile | Exit 0 with `-Wall -Wextra -Werror` |
| Updated native-secure P1 lifecycle | Exit 0; fresh server release verified after graceful departures, capacity/refusal/crash recovery/fault cases and audit verified |
| Explicit injected ACK timeout | Exit 0; warning retained, client exits normally, real server/audit Leave observed; this is not evidence of native ACK reliability |
| Merge preservation | All 22 original local changed/new files restored; 17 outside the five intentional conflict resolutions unchanged after newline normalization; backup and stash retained |

Selected actual output:

```text
SERVER_SUMMARY joins=13 rejects=1 leaves=12 timeouts=1 invalid=4 stale=1 peak=5 inputs=381 rate_hits=0 auth_failures=0 middleware_events=8 moves=2475 shots=210 destroyed=35 splits=19 hits=0 respawns=0 gameovers=0 restarts=0 wraps=0 snapshots=235 points=2280 ticks=709
PASS lifecycle audit records=38 root=3915581ee18bcfde3a32dd20084185f790b34ebb61fd63c9c67f6a83f2b53b00
PASS P1 networking lifecycle and malformed-message counters
CLIENT_LEAVE_ACK matched_before=1 matched_after=1 result=-10 requested_ns=1000000000 elapsed_ns=361 clock_valid=1
CLIENT_WARNING leave_ack_timeout: transport acknowledgement unconfirmed; server slot release requires independent verification
CLIENT_EXIT
```

The timeout fixture returns immediately by design; its 361 ns elapsed is not a native one-second timeout measurement. Its summary reports client/server cleanup exit 0, server/audit Leave observed and status PASS. Raw commands, output and exit statuses: `build/evidence/pull-480227c-20261105T030706Z/`. Historical native safety findings remain open; no GUI, full P2, sanitizer or performance result is implied by this pull check.

---

# P0 independent test report

Purpose: record environment and baseline acceptance evidence. Author: <team>, with Codex tester assistance. Date: 2026-11-03.

## Result

P0 BLOCKED. No baseline or game executable was built or launched. No source files, baseline files, packages or git configuration were modified by the tester.

| Check | Result | Evidence |
|---|---|---|
| Ubuntu WSL2 available | PASS | uname below |
| Compiler / make / Valgrind / OpenSSL executable | PASS | command paths below |
| pkg-config and Cyclone idlc | BLOCKED | Both missing |
| Cyclone DDS development/tools and OpenSSL development libraries | BLOCKED | dpkg-query says no packages found |
| GLFW/OpenGL development libraries | PASS (installed only) | libglfw3-dev and libgl1-mesa-dev have ii status |
| DDS Security plugins | UNDECIDED | Count 0 while Cyclone DDS itself is not installed; repeat after installation |
| WSLg configuration | PRESENT, runtime unverified | DISPLAY=:0, WAYLAND_DISPLAY=wayland-0; no GUI launched |
| ASLR | PASS | Value 2 |
| Interface multicast capability | PRESENT, delivery unverified | eth0 MULTICAST flag; no end-to-end multicast test |
| Baseline build and two clients moving squares | BLOCKED | Required development packages absent |
| Screenshot/input injection/sniff tools | BLOCKED | xvfb-run, import, xdotool, tcpdump missing |

Do not choose security fallback B solely from this unprovisioned environment. Recheck native DDS Security plugins after dependencies are installed.

## Reproduction

From PowerShell use `wsl -d Ubuntu`, then in Ubuntu:

```bash
for tool in gcc make pkg-config idlc valgrind openssl readelf xvfb-run import xdotool tcpdump; do
  if command -v "$tool"; then :; else printf '%s: MISSING\n' "$tool"; fi
done
dpkg-query -W -f='${binary:Package}: ${db:Status-Abbrev} ${Version}\n' build-essential pkg-config pkgconf gcc make libssl-dev libglfw3-dev libgl1-mesa-dev cyclonedds-dev cyclonedds-tools
find /usr /opt -name 'libdds_security_*' 2>/dev/null | wc -l
printenv DISPLAY WAYLAND_DISPLAY XDG_RUNTIME_DIR
cat /proc/sys/kernel/randomize_va_space
ip link show eth0
```

The combined independent probe exited 0 (the final status message succeeds); this is not an aggregate dependency PASS. Its raw output is:

```text
=== P0 independent dependency probe ===
Linux Dain 6.6.87.2-microsoft-standard-WSL2 #1 SMP PREEMPT_DYNAMIC Thu Jun  5 18:30:46 UTC 2025 x86_64 x86_64 x86_64 GNU/Linux
/usr/bin/gcc
/usr/bin/make
pkg-config: MISSING
idlc: MISSING
/usr/bin/valgrind
/usr/bin/openssl
/usr/bin/readelf
xvfb-run: MISSING
import: MISSING
xdotool: MISSING
tcpdump: MISSING
=== dependency states ===
dpkg-query: no packages found matching pkgconf
dpkg-query: no packages found matching libssl-dev
dpkg-query: no packages found matching cyclonedds-dev
dpkg-query: no packages found matching cyclonedds-tools
build-essential: ii  12.10ubuntu1
gcc: ii  4:13.2.0-7ubuntu1
libgl1-mesa-dev:amd64: ii  25.2.8-0ubuntu0.24.04.1
libglfw3-dev:amd64: ii  3.3.10-1build1
make: ii  4.3-4.1build2
pkg-config: un
=== DDS security plugin count ===
0
=== display ===
:0
wayland-0
/run/user/1000/
ASLR=2
=== interface multicast flag ===
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc mq state UP mode DEFAULT group default qlen 1000
    link/ether 00:15:5d:0e:df:43 brd ff:ff:ff:ff:ff:ff
=== baseline build status ===
NOT RUN: required pkg-config, idlc and development libraries are missing.
```

An earlier probe accidentally sent CRLF to bash stdin and its dry-run target became `baseline\r`; that attempt is a command-transport artefact, not evidence of a repository build defect. The normalized probe above supersedes it. No actual make baseline build was attempted.

## Human environment action

Run these inside Ubuntu (agents did not execute them):

```bash
sudo apt update
sudo apt install -y build-essential pkg-config git gdb valgrind openssl libssl-dev libgl1-mesa-dev libglfw3-dev cyclonedds-dev cyclonedds-tools xvfb xauth xdotool imagemagick tcpdump
```

The command includes already installed baseline dependencies for reproducibility. Missing pkg-config, cyclonedds-dev, cyclonedds-tools and libssl-dev block development. The GUI/capture packages enable required screenshot, input and packet-capture verification. Package-install success is not claimed. After installation rerun P0 checks, build `make baseline`, launch server then two clients, and prove movement in both windows.

## Remaining suite

CODEX_GOAL verification items 1–8 have not passed. Full game build, sanitizer checks, unit tests, headless soak, gameplay, lease, security negatives, packet confidentiality, GUI game rendering and human playtest remain pending later phases.

## P0 resumed after human dependency installation — PASS (2026-11-03)

This section supersedes the initial environment blocker. It does not erase the initial observations. Builder reported `make baseline` exit 0; tester independently ran the resulting unchanged baseline server and two distinct client processes under Xvfb. Baseline compilation has warnings in baseline/third_party; a zero-warning game build is not claimed.

Independent dependency probe exited 0 and produced:

```text
=== resumed dependencies ===
0.10.4
3.3.10
3.0.13
/usr/bin/idlc
/usr/bin/xvfb-run
/usr/bin/import
/usr/bin/xdotool
/usr/bin/tcpdump
/usr/lib/x86_64-linux-gnu/libddsc0debian/libdds_security_crypto.so
/usr/lib/x86_64-linux-gnu/libddsc0debian/libdds_security_ac.so
/usr/lib/x86_64-linux-gnu/libddsc0debian/libdds_security_auth.so
=== python ===
/usr/bin/python3
```

Version order is CycloneDDS, GLFW, OpenSSL from `pkg-config --modversion CycloneDDS glfw3 openssl`. Security Option A is now the preferred implementation path because all three native plugins are present. Plugin presence is not proof that authentication or encryption is configured or working.

### Independent two-client GUI test

Reproduction from Ubuntu repository root (scripts and screenshots are ignored local evidence under build/):

```bash
xvfb-run -a -s "-screen 0 1620x650x24" bash build/p0-test/gui.sh
python3 build/p0-test/pixels.py
```

The GUI script exports `LIBGL_ALWAYS_SOFTWARE=1`, launches server first, waits for each client's join response sequentially, finds each X11 window using PID AND title (`xdotool search --all`), captures both views before input, focuses player 1 and holds Right for 0.25 seconds, captures both, focuses player 2 and holds Down for 0.25 seconds, then captures both. It checks all three processes remain alive and kills only its own processes in an EXIT trap. It does not edit baseline source.

Final GUI run exited 0 with raw output:

```text
Processes alive after input: server + two clients
Client1 joins: 1
Client2 joins: 1
Client1 inputs: 26
Client2 inputs: 27
Client1 positions: 122
Client2 positions: 122
Server join lines:
Join request received
Join response sent
Join request received
Join response sent
Screenshots:
build/p0-test/client1-after-p1.png PNG 800x600 800x600+0+0 8-bit sRGB 2c 398B 0.000u 0:00.003
build/p0-test/client1-after-p2.png PNG 800x600 800x600+0+0 8-bit sRGB 2c 398B 0.000u 0:00.002
build/p0-test/client1-before.png PNG 800x600 800x600+0+0 8-bit sRGB 2c 407B 0.000u 0:00.003
build/p0-test/client2-after-p1.png PNG 800x600 800x600+0+0 8-bit sRGB 2c 405B 0.000u 0:00.003
build/p0-test/client2-after-p2.png PNG 800x600 800x600+0+0 8-bit sRGB 2c 405B 0.000u 0:00.003
build/p0-test/client2-before.png PNG 800x600 800x600+0+0 8-bit sRGB 2c 411B 0.000u 0:00.003
```

Screenshots inspected with view_image: client1-before, client2-before, client1-after-p1, client2-after-p2. The two windows show distinct Player 1 / Player 2 HUDs and both red square outlines on white backgrounds. Player 1 moves right; player 2 moves down. The pixel assertion scans 800x600 RGB data for 50-pixel horizontal red square edges below the HUD, removes bottom edges with a one-pixel raster endpoint tolerance, requires exactly two squares per frame, requires a change after each player's input in both clients, and requires both clients agree on final positions. Failure of any condition exits nonzero.

Final pixel assertion exited 0:

```text
client1: before=[(251, 130), (677, 418)], after_p1=[(270, 130), (677, 418)], after_p2=[(270, 130), (677, 438)]
PASS: both player movements visible in client1
client2: before=[(251, 130), (677, 418)], after_p1=[(270, 130), (677, 418)], after_p2=[(270, 130), (677, 438)]
PASS: both player movements visible in client2
PASS: clients agree on final square positions
```

Harness corrections: the initial GUI run exited 1 because xdotool's default OR search selected the same window for both PIDs, leaving client2 with zero inputs. Changed to --all and repeated from fresh processes. The initial pixel assertion exited 1 because the bottom edge's raster start differs by one pixel; corrected edge pairing tolerance and reran. These failures were in test harnesses; neither was hidden or treated as a baseline pass.

| P0 acceptance | Result |
|---|---|
| Baseline built | PASS, builder command evidence (warnings acknowledged) |
| Server and two distinct client processes run | PASS, independent GUI script |
| Both windows display both squares and distinct player HUD | PASS, screenshot inspection |
| Each player moves, and both clients agree | PASS, input logs and pixel assertions |
| Native DDS Security plugin check | PASS, plugins installed; Option A gate met |
| Same-host DDS discovery and data delivery | PASS for default configuration in this run |
| Cross-machine, mirrored/NAT networking or multicast-specific delivery | NOT TESTED |
| WSLg real desktop runtime | NOT TESTED; Xvfb software OpenGL exercised |

P0 baseline acceptance is satisfied. Full game suite items 1–8 remain pending implementation; this is not an asteroid game rendering or security pass. No private keys were generated or printed.

## P1 independent integration — PASS (initial runtime, 2026-11-03)

The P1 binaries explicitly report INSECURE_P1. These results verify networking lifecycle and validation, not mutual authentication, confidentiality, integrity against active attackers, or cryptographic replay protection. Native DDS Security remains a P2 task.

Tester added `tests/p1_network.py`, using only subprocesses and the public CLI/log output. It fails nonzero on missing expected output, reused admission nonce/slot, early process death, wrong killed-slot reclamation, missing individual fault event, failed exit code, or insufficient server summary counters. The script owns and cleans up only its child processes. No package installs or baseline edits occurred.

Reproduction from Ubuntu repository root:

```bash
python3 tests/p1_network.py
make test
```

The integration command exited 0 on its first P1 runtime execution. It starts a server with --duration 60, launches five distinct identities concurrently with --headless --bot circle --duration 45, verifies five distinct nonces and player IDs, then verifies a sixth identity is rejected. After one graceful leave it accepts a replacement and keeps that replacement alive, so five slots are occupied before SIGKILL. It kills identity 2, waits 3.5 seconds, and requires identity 2 to rejoin the exact killed player's former slot while the other four clients remain alive. Thus the timeout test cannot pass simply by using an already-free slot.

Next, each fault runs from identity 6 for two seconds: spoof, session, id-zero, id-high, replay. Each must produce its own exact SERVER_DROP event; aggregate counters alone are insufficient. All remaining legitimate clients must survive and gracefully exit 0. The server must exit 0 with joins>=12, rejects>=1, leaves>=10, timeouts>=1, invalid>=4, stale>=1, inputs>=1 and peak==5.

Raw integration stdout:

```text
COMMAND /mnt/c/Users/jorda/Desktop/Networking_CSD3116/build/release/server --duration 60
COMMAND /mnt/c/Users/jorda/Desktop/Networking_CSD3116/build/release/client --identity 1 --headless --bot circle --duration 45
COMMAND /mnt/c/Users/jorda/Desktop/Networking_CSD3116/build/release/client --identity 2 --headless --bot circle --duration 45
COMMAND /mnt/c/Users/jorda/Desktop/Networking_CSD3116/build/release/client --identity 3 --headless --bot circle --duration 45
COMMAND /mnt/c/Users/jorda/Desktop/Networking_CSD3116/build/release/client --identity 4 --headless --bot circle --duration 45
COMMAND /mnt/c/Users/jorda/Desktop/Networking_CSD3116/build/release/client --identity 5 --headless --bot circle --duration 45
PASS concurrent joins: five distinct nonces and slots
COMMAND /mnt/c/Users/jorda/Desktop/Networking_CSD3116/build/release/client --identity 6 --headless --bot circle --duration 3
PASS sixth client rejected
COMMAND /mnt/c/Users/jorda/Desktop/Networking_CSD3116/build/release/client --identity 6 --headless --bot circle --duration 45
PASS explicit leave permits replacement
COMMAND /mnt/c/Users/jorda/Desktop/Networking_CSD3116/build/release/client --identity 2 --headless --bot circle --duration 45
PASS SIGKILL lease expiry permits rejoin
COMMAND /mnt/c/Users/jorda/Desktop/Networking_CSD3116/build/release/client --identity 6 --headless --bot circle --duration 2 --fault spoof
PASS SERVER_DROP identity=6 reason=spoof
COMMAND /mnt/c/Users/jorda/Desktop/Networking_CSD3116/build/release/client --identity 6 --headless --bot circle --duration 2 --fault session
PASS SERVER_DROP identity=6 reason=session
COMMAND /mnt/c/Users/jorda/Desktop/Networking_CSD3116/build/release/client --identity 6 --headless --bot circle --duration 2 --fault id-zero
PASS SERVER_DROP identity=6 reason=id-zero
COMMAND /mnt/c/Users/jorda/Desktop/Networking_CSD3116/build/release/client --identity 6 --headless --bot circle --duration 2 --fault id-high
PASS SERVER_DROP identity=6 reason=id-high
COMMAND /mnt/c/Users/jorda/Desktop/Networking_CSD3116/build/release/client --identity 6 --headless --bot circle --duration 2 --fault replay
PASS SERVER_DROP identity=6 reason=replay
SERVER_SUMMARY joins=12 rejects=1 leaves=11 timeouts=1 invalid=76 stale=18 peak=5 inputs=488
PASS P1 networking lifecycle and malformed-message counters
```

Per-process raw stdout is retained in ignored `build/p1-test/*.log`. Logs use P1 numeric identity/session routing; they do not establish authenticated identity. No adversarial cryptographic tests or sanitizers were run in this phase.

Independent `make test` exited 0 with raw output:

```text
mkdir -p build/release
cc -std=gnu11 -O2 -g -Wall -Wextra -Wformat=2 -Wformat-security -fstack-protector-strong -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE  -Isrc -Ibuild/gen -isystem third_party   tests/test_protocol.c src/common/net.c src/common/options.c src/common/protocol.c build/gen/messages.c -o build/release/test-protocol -pie -Wl,-z,relro,-z,now  -lddsc  -lssl -lcrypto  -pthread -lm
./build/release/test-protocol
PASS protocol boundaries and CLI validation
```

`tests/test_protocol.c` uses live assert calls (no NDEBUG in the shown compile command) and exercises NULL, zero/high player IDs, key flags, zero session/sequence, protocol versions, oversized and NULL-backed snapshot sequences, duplicate IDs, coordinate bounds and invalid CLI identity. These assertions can terminate nonzero. P1 integration has no runtime FAILs; its earlier syntax-only check generated a Python cache file, which was removed by exact path.

| P1 check | Independent result |
|---|---|
| Concurrent addressed joins / distinct slots | PASS |
| Five-player cap and sixth rejection | PASS |
| Explicit leave then replacement | PASS |
| SIGKILL exact-slot timeout/rejoin | PASS |
| Spoof/session/zero/high-ID/replay events | PASS for P1 validators |
| Legitimate clients/server survive fault runs | PASS |
| Unit boundary/CLI validation | PASS |
| Full clean game/baseline rebuild | Builder PASS; not independently repeated by tester during this run |
| Authenticated identity / encrypted traffic / active-attack replay defence | NOT IMPLEMENTED OR CLAIMED IN P1 |
| GUI asteroid gameplay / sanitizer soak / two-machine networking | PENDING later phases |

### P1 final regression after reviewer fixes — PASS

Reviewer found a HIGH unsigned lease-age underflow: a main-thread timestamp sampled before locking could precede a newly updated worker heartbeat timestamp. Builder guarded `now >= seen` in `lease_expired` and added deterministic unit coverage, and wired additional Join/Response/snapshot validators. This runtime change justified repeating the independent suite once. Tester waited for builder's rebuild-ready signal.

Command:

```bash
python3 tests/p1_network.py && make test
```

Combined exit code: 0. All command/phase lines through `--fault id-zero` matched the initial transcript. Final raw output:

```text
PASS SERVER_DROP identity=6 reason=id-zero
COMMAND /mnt/c/Users/jorda/Desktop/Networking_CSD3116/build/release/client --identity 6 --headless --bot circle --duration 2 --fault id-high
PASS SERVER_DROP identity=6 reason=id-high
COMMAND /mnt/c/Users/jorda/Desktop/Networking_CSD3116/build/release/client --identity 6 --headless --bot circle --duration 2 --fault replay
PASS SERVER_DROP identity=6 reason=replay
SERVER_SUMMARY joins=12 rejects=1 leaves=11 timeouts=1 invalid=76 stale=18 peak=5 inputs=487
PASS P1 networking lifecycle and malformed-message counters
./build/release/test-protocol
PASS protocol boundaries and CLI validation
```

This is the final P1 evidence after the lease fix. Local per-process logs in build/p1-test now correspond to this latest run. No remaining independent runtime failure was observed. P1 remains explicitly insecure pending P2.


## P2 sequential fallback verification — checkpoint, not phase completion

All three real subagents returned a runtime usage-limit error. SUBAGENTS_UNAVAILABLE. Orchestrator continued the builder, reviewer and tester roles sequentially, re-reading actual code between roles; these last checks are not claimed as independent-agent review.

Builder role: verified saved code, built native-secured executables and shim. A second P2 run hung on a non-reading response peer: process.wait(timeout=20) expired, cleanup needed kill after another5 seconds. Replaced Response KEEP_ALL history with bounded reliable KEEP_LAST(1) on both endpoints, preserving idempotent latest join response; other reliable control topics retain bounded KEEP_ALL. Corrected test cleanup to continue through all owned children. Reviewer role freshly re-read endpoint QoS, response timeout handling, rate buckets and test assertions. Tester role then ran the final suite.

Commands inside Ubuntu repo root:
```bash
make -j2 test all
cc -shared -fPIC -Wall -Wextra tests/rtps_fault.c -o build/rtps_fault.so -ldl -pthread -lcrypto
python3 tests/p2_security.py --security-dir /tmp/dds-asteroids.7unxZ8xi
```
Build and shim compile exit0, no warnings. Unit output:
```text
PASS protocol boundaries and CLI validation
```
P2 harness exit0. Actual assertion output (COMMAND lines omitted; full per-process output remains build/p2-test):
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
Server log tail:
```text
DDS_DIAGNOSTIC category=128 reason=decode function=q_omg_security_decode_rtps_message cause=gcm-final-failed
DDS_DIAGNOSTIC category=128 reason=decode function=q_omg_security_decode_rtps_message cause=gcm-final-failed
SERVER_LEAVE identity=6 id=2
SERVER_LEAVE identity=1 id=1
SERVER_SUMMARY joins=6 rejects=45 leaves=5 timeouts=1 invalid=0 stale=0 peak=2 inputs=511 rate_hits=4455 auth_failures=6 middleware_events=16
AUDIT_ROOT hash=f48ff0db315e9af259b3eef248891028b019ef9aed389c47979750342aab90ce records=117
WIRE_FAULT_SUMMARY calls=1075 mutations=0
NATIVE_GCM_SUMMARY failures=10
```
The server shim observed real OpenSSL GCM finalization failure (test instrumentation). Separately, the production server native trace sink emitted decode/gcm-final-failed diagnostics and chained DDS_DECODE_WARNING records. No pre-encryption payload mutation is substituted for ciphertext corruption. Serialized before/after datagrams differ by one bit and the altered sendmsg returned336 bytes. This is process-local serialized outgoing-packet evidence, not an external Wireshark capture.

After the Response QoS change, secure lifecycle regression command also exited0:
```bash
python3 tests/p1_network.py --security-dir /tmp/dds-asteroids.7unxZ8xi
```
Actual final output:
```text
PASS SERVER_DROP identity=6 reason=session
PASS SERVER_DROP identity=6 reason=id-zero
PASS SERVER_DROP identity=6 reason=id-high
PASS SERVER_DROP identity=6 reason=replay
SERVER_SUMMARY joins=12 rejects=1 leaves=11 timeouts=1 invalid=55 stale=13 peak=5 inputs=557 rate_hits=0 auth_failures=0 middleware_events=9
PASS P1 networking lifecycle and malformed-message counters
```
Earlier lines of that same run passed five concurrent distinct nonce/slot joins, sixth refusal, explicit replacement, exact killed-slot reclamation and spoof rejection. The source test requires each assertion, not exit0 alone.

Minimal credential export command exited0:
```bash
bash tools/export_identity.sh /tmp/dds-asteroids.7unxZ8xi client1
stat -c '%a %u' /tmp/dds-participant.VOz8CSGL /tmp/dds-participant.VOz8CSGL/identities/client1.key
bash tools/check_secrets.sh
```
Actual output:
```text
SECURITY_DIR=/tmp/dds-participant.VOz8CSGL
700 1000
600 1000
PASS no tracked .key/.pem paths
```
Export includes only client1 private key/certificate, public CA certificates, signed governance and client1 permissions. No CA/server private key is copied. No key contents were read or printed. git diff --name-only -- baseline third_party returned empty output.

External capture attempt, command `timeout 2 tcpdump -i lo -c 1 -nn`, exited1:
```text
tcpdump: lo: You don't have permission to perform this capture on that device
(socket: Operation not permitted)
```
Human capture, if needed, in a separate Ubuntu terminal while secured server/clients run:
```bash
sudo tcpdump -i lo -nn -s 0 -w /tmp/dds-asteroids-capture.pcap 'udp portrange 17900-17941'
```
Stop capture with Ctrl-C. Inspect in Wireshark. Public discovery metadata may remain visible; inspect protected gameplay/input bytes, not only packet labels. This human command was not executed by the agent.

### Remaining acceptance gap — do not mark P2 complete
The no-cert client did not obtain a slot and exited1, but the first native authentication audit records appeared during the subsequent wrong-CA attempt. No correlated no-cert server audit event was observed. tests/p2_security.py denial_cases asserts lack of admission and survival, not an event delta per negative. It therefore does NOT prove the GOAL requirement that every negative has an expected rejection in the server audit log. This is an observability/test-coverage gap, not evidence of authentication bypass. Resolve native unauthenticated-peer rejection observability and add per-case assertions before claiming full P2 acceptance.

Leave limiting is implemented with a separate bucket but its dedicated flood runtime regression is still missing. Input-flood movement-speed comparison and protected gameplay snapshot evidence depend on P3, which is unimplemented. P4 sanitizer/Valgrind/soak/hardening verification, P5 measurements and P6 materials are pending. No complete game, clean-clone verification or final definition-of-done claim is made.

## Resumed independent testing — 2026-11-04 (P3 in progress)

User authorized renewed implementation after the prior checkpoint. The real tester runtime is available again. Earlier sequential fallback evidence remains labeled as such.

Tester added independent `tests/test_sim.c` behavior fixtures and compiled them directly against the builder's new pure `src/common/sim.c`, before network/renderer readiness. Actual Ubuntu command:

```bash
mkdir -p build/p3-test
cc -std=gnu11 -O2 -g -Wall -Wextra -Wformat=2 -fstack-protector-strong -fPIE -Isrc tests/test_sim.c src/common/sim.c -o build/p3-test/test-sim -pie -Wl,-z,relro,-z,now -lm
./build/p3-test/test-sim
```

Exit 0, no compiler warnings. Raw stdout:

```text
PASS sim wrap, speed limit and rotation
PASS sim fire cadence, full bullet pool and leave cleanup
PASS sim splitting, owner score and seam collision
PASS sim damage, exact respawn delay and invulnerability
PASS sim game-over and explicit restart
PASS sim capped split pool and guard words
PASS independent simulation behavior fixtures
```

These fixtures check observable state independently of counters: two medium children after a large asteroid collision; owner-only score; a small asteroid destroyed across the wrap seam; one life lost for overlapping contacts; no early respawn before its full configured delay; invulnerability after respawn; collision-driven defeat and explicit restart restoring lives/wave/score while preserving evidence counters; full pools remain capped and adjacent guard words unchanged. Held fire for sixty authoritative ticks yields five shots. They are pure simulation tests, not a network or GUI pass.

Prepared `tests/p3_gameplay.py` and `tests/p3_gui.py` await the new game binaries. The former requires five secured clients, authoritative movement, destruction, scores, life changes and bounded snapshots; the latter drives real GLFW keys in two distinct Xvfb windows for at least ten seconds and retains screenshots for visual inspection. No runtime gameplay or visual acceptance is claimed in this interim entry.

P2 test strengthening: `tests/p1_network.py` now verifies each secured malformed-input DROP in the chained audit and checks the final audit against its retained root. `tests/p2_security.py` now requires new byte-offset-correlated audit records per wrong-CA, no-cert discovery, rate and ciphertext test. No-cert discovery is explicitly an observation of nonsecure SPDP followed by denied admission, not a certificate verification failure. A forbidden writer denied locally by native access control cannot generate an unseen remote server event; the test requires the genuine local denial and no new server join instead.

## 2026-11-04 — orchestrator local launcher verification
Command inside Ubuntu repo root:
```bash
bash -n tools/play_local.sh
bash tools/play_local.sh --headless --duration 5
```
Both exit0. Launcher compiled latest client warning-free, generated a private Linux fixture, validated public signed permissions/expiry plus root/key modes, started server+two bot clients, and boundedly cleaned its own jobs. OpenSSL printed only public certificate subjects, no private key material. Server tail:
```text
SERVER_JOIN identity=2 id=1 status=accepted
SERVER_JOIN identity=1 id=2 status=accepted
SERVER_SUMMARY joins=2 rejects=0 leaves=0 timeouts=0 invalid=0 stale=0 peak=2 inputs=84 rate_hits=0 auth_failures=0 middleware_events=5 moves=502 shots=42 destroyed=10 splits=7 hits=0 respawns=0 gameovers=0 restarts=0 wraps=0 snapshots=100 points=560 ticks=300
AUDIT_ROOT hash=1b40b009adada6470693931caf4252e2ab2c74e658830162aae61d8cb13f0037 records=4
```
Both client logs end CLIENT_EXIT; process inventory after exit contains no game/server/client children. The five-second server duration naturally ended before both clients' Leave; leaves0 is not presented as a Leave regression. Independent lifecycle tests cover that separately. Log paths: build/local-server.log, build/local-client1.log, build/local-client2.log. build/local-security-dir contains a path only, not credentials.

Orchestrator also viewed actual tester screenshots build/p3-gui/client1-after-input.png and client2-10s.png. Both show two colored ship outlines, irregular asteroid outlines, bullet points, player identities, DDS secure connected label, score/lives, wave, RTT and arrow/space/R control footer. Some rocks cross the HUD area (LOW visual limitation); no blank/missing arena or unreadable main HUD observed. Final targeted GUI rerun pending builder freshness/rejection-HUD corrections.

## 2026-11-04 — final independent P2 regression on the game

Commands in Ubuntu repository root, using the existing private Linux development fixture (path only, no key contents):

```bash
python3 tests/p2_security.py --security-dir /tmp/dds-asteroids.7unxZ8xi
python3 tests/p1_network.py --security-dir /tmp/dds-asteroids.7unxZ8xi
```

Both final commands exited 0. Each P2 case records the audit file byte offset before launch; only newly appended matching records can satisfy that case. Lack of an accepted client response alone is insufficient: no-cert/wrong-CA/forbidden-writer cases also require no new identity6 JOIN audit record (no slot allocated).

Final security assertion stdout (COMMAND lines omitted; exact arguments and per-process stdout retained in tests/p2_security.py and build/p2-test):

```text
PASS new audit event identity=0 event=DDS_UNAUTHENTICATED_DISCOVERY reason=nonsecure-spdp
PASS no-cert: no accepted join, exit=1; see native diagnostics in build/p2-test/no-cert.log
PASS new audit event identity=0 event=DDS_AUTH_WARNING reason=authentication
PASS wrong-ca: no accepted join, exit=1; see native diagnostics in build/p2-test/wrong-ca.log
PASS rogue-server: no accepted join, exit=1; see native diagnostics in build/p2-test/rogue-server.log
PASS slow reader: 60 responses; legitimate player and server survive
PASS fresh authenticated join after slow-reader departure
PASS new audit event identity=6 event=RATE reason=input
PASS SERVER_RATE identity=6 kind=input
PASS new audit event identity=6 event=RATE reason=join
PASS SERVER_RATE identity=6 kind=join
PASS new audit event identity=6 event=RATE reason=leave
PASS SERVER_RATE identity=6 kind=leave
PASS new audit event identity=0 event=DDS_DECODE_WARNING reason=decode
PASS actual ciphertext send: byte=52 packet_bytes=344
PASS receiver OpenSSL GCM authentication failure observed
PASS flood tick bound and honest snapshot progress
PASS audit chain records=136 root=ae80796d4a87e3cc0118fb217fcded64b7622cc733c11bcc32be2c264c34afcd
PASS audit modified detected: Record 1: content hash mismatch
PASS audit truncated detected: Final root differs from trusted server root
PASS P2 harness assertions; native rejection/rate details require report evidence
```

Final server tail:

```text
DDS_DIAGNOSTIC category=128 reason=decode function=q_omg_security_decode_rtps_message cause=gcm-final-failed
SERVER_LEAVE identity=6 id=2
SERVER_LEAVE identity=1 id=1
SERVER_SUMMARY joins=7 rejects=45 leaves=6 timeouts=1 invalid=14 stale=0 peak=2 inputs=592 rate_hits=6570 auth_failures=6 middleware_events=17 moves=3207 shots=270 destroyed=44 splits=23 hits=0 respawns=0 gameovers=0 restarts=0 wraps=0 snapshots=847 points=2980 ticks=2584
AUDIT_ROOT hash=ae80796d4a87e3cc0118fb217fcded64b7622cc733c11bcc32be2c264c34afcd records=136
WIRE_FAULT_SUMMARY calls=2675 mutations=0
NATIVE_GCM_SUMMARY failures=10
```

The native receiver rejected actual corrupted encrypted RTPS: the sending shim changed precisely one bit at byte52 of a 344-byte post-encryption datagram and real sendmsg returned344. Sender logged10 mutations; separately the receiver's real OpenSSL GCM finalization returned failure10 times. The production trace filter also emitted newly chained DDS_DECODE_WARNING records. This is genuine encryption/integrity evidence from process-local socket instrumentation; it is not an external privileged packet capture or a claim that ciphertext is self-evident from visual randomness alone.

No-cert semantics: the native trace observes nonsecure participant discovery, which is logged with identity0 because no authenticated identity exists. Admission stays denied. This is not misreported as a certificate verification failure. Wrong-CA native handshake warnings are actual authentication failures. Forbidden authoritative publication is refused by local native access control before a forbidden sample can reach the server; the genuine `CLIENT_FORBIDDEN_WRITER result=denied` and absence of server JOIN are asserted. A remote server cannot audit an attempt that never left another process, so no fabricated remote rejection event is claimed.

Leave-flood regression deliberately sends invalid-session Leave samples while retaining ordinary heartbeats. It produces a separate RATE/leave audit and legitimate peer snapshots continue advancing. The flooder's own final valid Leave can be dropped after its bucket is exhausted; the harness now waits its documented3-second lease before reusing that identity. The first strengthened attempt failed because it reused identity6 immediately, received a legitimate rejection of the still-live prior session, and stopped before the wire test. The corrected final attempt above passes. This was a test-precondition correction, not a bypass or crash fix.

Flood movement evidence is scoped: server moves3207 <= ticks2584 * maxplayers5, live authoritative velocity samples remain <=200px/s (hundredth-pixel wire units with rounding tolerance), and the honest peer receives later simulation ticks during each input/join/Leave flood. Pure physics fixtures independently test the speed cap and once-per-tick movement. This is a structural and observed-speed bound; it is not a measured equal-path distance comparison under identical delivery schedules.

Final secured lifecycle also asserts each malformed-input rejection in the chained audit (spoof, session, id-zero, id-high, replay), five distinct nonces/slots, sixth refusal, leave replacement and exact killed-slot reclamation. Raw final tail:

```text
PASS SERVER_DROP identity=6 reason=session
PASS SERVER_DROP identity=6 reason=id-zero
PASS SERVER_DROP identity=6 reason=id-high
PASS SERVER_DROP identity=6 reason=replay
SERVER_SUMMARY joins=12 rejects=1 leaves=11 timeouts=1 invalid=52 stale=12 peak=5 inputs=553 rate_hits=0 auth_failures=0 middleware_events=6 moves=3596 shots=303 destroyed=51 splits=25 hits=0 respawns=0 gameovers=0 restarts=0 wraps=0 snapshots=350 points=3580 ticks=1061
PASS lifecycle audit records=93 root=766731f6fcf6abd90fcdc6aa3adb105781eb5eeda21b96bef11aa4d91278bf62
PASS P1 networking lifecycle and malformed-message counters
```

## 2026-11-04 — independent P3 gameplay and visual acceptance

Command:

```bash
python3 tests/p3_gameplay.py --security-dir /tmp/dds-asteroids.7unxZ8xi
```

Exit0. This is a normal35-second encrypted session with five hunter bots sending ordinary key states; no controlled scenario flag is enabled. Each client requires changing authoritative ticks and ship positions, bounded coordinates, multiple snapshots, and successful exit. At least one received score must be positive and a life count must drop. Server counters independently require movement, firing, destroyed asteroids, splits, hits, respawns and wrapping.

Raw output:

```text
PASS client1.log: snapshots=34 maxscore=3170 minlives=2
PASS client2.log: snapshots=34 maxscore=3710 minlives=2
PASS client3.log: snapshots=34 maxscore=3380 minlives=2
PASS client4.log: snapshots=34 maxscore=2520 minlives=2
PASS client5.log: snapshots=34 maxscore=2820 minlives=1
SERVER_SUMMARY joins=5 rejects=0 leaves=5 timeouts=0 invalid=0 stale=0 peak=5 inputs=1697 rate_hits=0 auth_failures=0 middleware_events=0 moves=9831 shots=831 destroyed=223 splits=100 hits=7 respawns=6 gameovers=0 restarts=0 wraps=30 snapshots=704 points=16280 ticks=2134
PASS authoritative gameplay effects
```

A follow-up assertion over the170 retained authoritative velocity samples also exited0:

```text
PASS live hunter snapshot velocity cap: samples= 170
```

The bound is `(vx*vx + vy*vy) <= 20002*20002` in hundredth-pixel/second units, allowing integer rounding around the200px/s cap. Logs are build/p3-gameplay/client1.log through client5.log and server.log. Normal gameplay produced no game-over within35seconds; the next controlled test explicitly verifies that rare transition rather than inventing normal-session counts.

### Controlled live game-over/restart

```bash
python3 tests/p3_gameover.py --security-dir /tmp/dds-asteroids.7unxZ8xi
```

Exit0. This dedicated test requests the off-by-default server-only `--test-scenario gameover`: after two actual admissions and180ticks it prepares low-life ships and overlapping rocks, then normal collisions derive death/game-over and the clients send ordinary restart inputs. The setup does not write evidence counters. This run is labeled controlled, not natural-session frequency evidence.

Raw output:

```text
SERVER_TEST_SCENARIO name=gameover tick=181
GAME_EVENT gameover tick=182
GAME_EVENT restart tick=192
SERVER_SUMMARY joins=2 rejects=0 leaves=2 timeouts=0 invalid=0 stale=0 peak=2 inputs=186 rate_hits=0 auth_failures=0 middleware_events=4 moves=1085 shots=93 destroyed=29 splits=16 hits=2 respawns=0 gameovers=1 restarts=1 wraps=3 snapshots=210 points=1920 ticks=632
PASS controlled encrypted collision/game-over/ordinary-input restart
```

Both independent client logs contain:

```text
CLIENT_GAME_STATE tick=184 gameover=1 wave=1
CLIENT_GAME_STATE tick=194 gameover=0 wave=1
```

The harness also verifies ordered setup<game-over<restart ticks, hits>=2, successful clients/server, and the audit against the final retained root.

### Two real GLFW windows and input

```bash
xvfb-run -a -s "-screen 0 1620x650x24" python3 tests/p3_gui.py --security-dir /tmp/dds-asteroids.7unxZ8xi
```

Initial run exited0; after builder corrected connected-state freshness/own-slot checks, the targeted final GUI run also exited0. The two clients use distinct PID-and-title matched X11 windows. Tester sends real Up/Right/Space events for a second to each client and keeps both windows alive for at least10.5seconds. It captures each window before keys, after keys, and after10seconds. Final raw output:

```text
SCREENSHOT build/p3-gui/client1-before.png
SCREENSHOT build/p3-gui/client2-before.png
SCREENSHOT build/p3-gui/client1-after-input.png
SCREENSHOT build/p3-gui/client2-after-input.png
SCREENSHOT build/p3-gui/client1-10s.png
SCREENSHOT build/p3-gui/client2-10s.png
SERVER_SUMMARY joins=2 rejects=0 leaves=2 timeouts=0 invalid=0 stale=0 peak=2 inputs=219 rate_hits=0 auth_failures=0 middleware_events=5 moves=788 shots=11 destroyed=3 splits=3 hits=2 respawns=2 gameovers=0 restarts=0 wraps=0 snapshots=250 points=90 ticks=753
PASS secure two-GUI lifecycle, real key input and >=10s rendering; inspect screenshots
```

Tester opened final client1-after-input.png and client2-10s.png with view_image. Confirmed nonblank800x600 arena, distinct mint/blue triangular ships with thrust accents, irregular asteroid outlines at several sizes, bullet points, CONNECTED / DDS SECURE P1 or P2, wave, player score/lives, RTT and arrow/space/R controls. Both players share authoritative changes; the server reports real shots/destruction/damage/respawn after the GUI key input. Respawn invulnerability can blink a ship off in an individual frame. LOW visual limitation: rocks can cross the title/HUD because they share the arena. Smoothness, controls feel and two-machine interaction still require human playtest.

| P3 acceptance | Independent evidence |
|---|---|
| Pure collisions, score/splits, bounds, pools, respawn, invulnerability | Deterministic behavior fixtures PASS |
| Five encrypted real clients / actual gameplay effects |35-second ordinary hunter session PASS |
| Lives/score/positions propagated |170 authoritative snapshot samples PASS |
| Real authoritative defeat and ordinary-input restart | Controlled two-client scenario PASS |
| Two GUI windows, real keys, graphics/HUD>=10seconds | Final Xvfb run + visual inspection PASS |
| Security regression + per-case audit / Leave flood / real ciphertext failure | Final P2 regression PASS with native/local distinction above |
| ASan/UBSan/TSan/Valgrind /120-second soak / full hardening | P4 pending; no pass claimed here |


## 2026-11-04 — fresh source-copy verification (orchestrator)

No git commit or clone was made. Copied current public sources into a new Linux temporary tree, excluding build outputs, git metadata, key/PEM files and runtime artifacts. This verifies generation/build from a clean source tree, not a literal committed clean clone. Command/output follows; exit0, no compiler warnings. Source-copy verification predates any subsequent P4 audit-throttle fix, which requires its own targeted rebuild/regression.

```text
CLEAN_SOURCE_ROOT=/tmp/dds-clean-source.v2a1bjb_
COMMAND=make -j2 all test check-hardening
mkdir -p build/gen
mkdir -p build/release
cd build/gen && idlc -l c ../../idl/messages.idl
cc -std=gnu11 -O2 -g -Wall -Wextra -Wformat=2 -Wformat-security -fstack-protector-strong -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE  -Isrc -Ibuild/gen -isystem third_party tests/test_sim.c src/common/sim.c -o build/release/test-sim -pie -Wl,-z,relro,-z,now,-z,noexecstack  -lm
mkdir -p build/release
cc -std=gnu11 -O2 -g -Wall -Wextra -Wformat=2 -Wformat-security -fstack-protector-strong -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE  -Isrc -Ibuild/gen -isystem third_party   src/server/main.c src/server/net_server.c src/common/audit_log.c src/common/frame.c src/common/net.c src/common/options.c src/common/protocol.c src/common/security.c src/common/sim.c build/gen/messages.c -o build/release/server -pie -Wl,-z,relro,-z,now,-z,noexecstack  -lddsc  -lssl -lcrypto  -pthread -lm
mkdir -p build/release
cc -std=gnu11 -O2 -g -Wall -Wextra -Wformat=2 -Wformat-security -fstack-protector-strong -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE  -Isrc -Ibuild/gen -isystem third_party    src/client/client_game.c src/client/main.c src/client/net_client.c src/client/render.c src/common/audit_log.c src/common/frame.c src/common/net.c src/common/options.c src/common/protocol.c src/common/security.c src/common/sim.c build/gen/messages.c -o build/release/client -pie -Wl,-z,relro,-z,now,-z,noexecstack  -lddsc  -lglfw  -lGL -lm -lssl -lcrypto  -pthread
mkdir -p build/release
cc -std=gnu11 -O2 -g -Wall -Wextra -Wformat=2 -Wformat-security -fstack-protector-strong -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE  -Isrc -Ibuild/gen -isystem third_party   tests/test_protocol.c src/common/audit_log.c src/common/frame.c src/common/net.c src/common/options.c src/common/protocol.c src/common/security.c src/common/sim.c build/gen/messages.c -o build/release/test-protocol -pie -Wl,-z,relro,-z,now,-z,noexecstack  -lddsc  -lssl -lcrypto  -pthread -lm
bash tools/check_hardening.sh build/release/server build/release/client
== build/release/server
  Type:                              DYN (Position-Independent Executable file)
  GNU_STACK      0x000000 0x0000000000000000 0x0000000000000000 0x000000 0x000000 RW  0x10
  GNU_RELRO      0x00a850 0x000000000000a850 0x000000000000a850 0x0007b0 0x0007b0 R   0x1
 0x000000000000001e (FLAGS)              BIND_NOW
 0x000000006ffffffb (FLAGS_1)            Flags: NOW PIE
PASS PIE NX RELRO BIND_NOW CANARY references=2
== build/release/client
  Type:                              DYN (Position-Independent Executable file)
  GNU_STACK      0x000000 0x0000000000000000 0x0000000000000000 0x000000 0x000000 RW  0x10
  GNU_RELRO      0x00d710 0x000000000000d710 0x000000000000d710 0x0008f0 0x0008f0 R   0x1
 0x000000000000001e (FLAGS)              BIND_NOW
 0x000000006ffffffb (FLAGS_1)            Flags: NOW PIE
PASS PIE NX RELRO BIND_NOW CANARY references=2
./build/release/test-protocol
PASS malformed bullet/asteroid/health snapshot boundaries
PASS protocol boundaries and CLI validation
./build/release/test-sim
PASS sim wrap, speed limit and rotation
PASS sim fire cadence, full bullet pool and leave cleanup
PASS sim splitting, owner score and seam collision
PASS sim damage, exact respawn delay and invulnerability
PASS sim game-over and explicit restart
PASS sim capped split pool and guard words
PASS independent simulation behavior fixtures
EXIT=0
```


## P4 checkpoint — hardening/ASan pass, dependency diagnostics unresolved

All three real agents reached a runtime usage limit again. SUBAGENTS_UNAVAILABLE. Root continued sequential builder/reviewer/tester roles with fresh actual code reads. It did not claim those last checks were independent-agent work.

### Build and hardening

Builder generated fail-closed tools/check_hardening.sh; independent reviewer weakened actual ELF fixtures and confirmed each missing protection fails, good stripped ELF passes, and first-bad/last-good aggregate remains failure (raw review evidence in REVIEW_LOG). Root ran final coherent release and ASan `make -j2 all test check-hardening`: exit0. Both show PIE, NX, RELRO, BIND_NOW, canary references2; unit/protocol/sim tests passed. TSan builds and ELF checks passed, while an unwrapped TSan test sometimes exited66 before main with unexpected-memory-mapping. Global ASLR was not changed.

Reviewer fixed a real narrow callback throttle race by guarding now<previous before unsigned subtraction and CAS. Final code was coherently rebuilt before root tests. No gameplay/network authority change or suppression was introduced.

### ASan + UBSan actual five-player churn

Real tester command:
```bash
python3 tests/p4_soak.py --security-dir /tmp/dds-asteroids.7unxZ8xi --bin-dir build/asan --duration 120
```
Tester reported exit0 after120s seeded churn:16 replacements,3 SIGKILL,13 graceful. Root independently re-read retained logs and verified the final trusted audit anchor; actual output:
```text
ASAN_REINSPECTION logs=23 diagnostics=0 audit_span_seconds=124.107
SERVER_SUMMARY joins=21 rejects=0 leaves=18 timeouts=3 invalid=0 stale=0 peak=5 inputs=5721 rate_hits=0 auth_failures=0 middleware_events=6 moves=31994 shots=2795 destroyed=912 splits=394 hits=27 respawns=24 gameovers=0 restarts=0 wraps=93 snapshots=2436 points=67540 ticks=7435
PASS retained ASan/UBSan churn artifacts and trusted audit root
```
The23 scanned .log files include22 process logs plus the audit file. No AddressSanitizer, LeakSanitizer or UBSan diagnostic was found. This is a two-minute test, not the ten-minute course soak.

After the final time-guard fix, root compiled the actual native audit integration probe with ASan/UBSan and ran separate finite secured/uncredentialed participants. Both exits0, actual output:
```text
PROBE_READY
DDS_DIAGNOSTIC reason=unauthenticated-discovery function=handle_spdp_alive cause=nonsecure-spdp
AUDIT_ROOT hash=a294cde99505b4b81125bf07c17c7160c965ab26205427701ce9870aa0bfd1cb records=1
PASS final audit time-guard native callback under ASan/UBSan; both exits0, real event, no diagnostics
```
Artifact: build/review/asan-probe-server.log. The first probe compile attempt was a PowerShell comma-argument transport error; quoting -fsanitize=address,undefined corrected it. That failure was not treated as a source-code or runtime pass.

### TSan actual-game failure — unsuppressed

Default TSan startup was unstable even for a one-pthread, joined-before-read minimal probe. Per-process `setarch x86_64 -R` consistently started that probe without changing global ASLR2. Root then ran:
```bash
setarch x86_64 -R make SAN=tsan test
python3 tests/p4_soak.py --security-dir /tmp/dds-asteroids.7unxZ8xi --bin-dir build/tsan --duration 120 --setarch
```
Wrapped unit tests exit0. The actual network soak FAILED after about9seconds: a gracefully stopped client returned66 due to real reported TSan warnings, so the harness stopped early and cleaned every child. It did not complete120seconds and is not a clean TSan session.

Observed warning summaries (runtime addresses omitted here; raw unmodified diagnostics remain build/p4-tsan-soak/*.log):
```text
WARNING: ThreadSanitizer: data race
SUMMARY: ThreadSanitizer: data race ... libddsc.so.0debian ... in ddsrt_mutex_destroy
SUMMARY: ThreadSanitizer: data race ... libddsc.so.0debian ... in ddsrt_recvmsg
ThreadSanitizer: reported 2 warnings
```
Visible conflicting accesses were library mutex destroy/unlock in gc/builtin threads and recvmsg/memcpy in DDS recv/builtin threads. Source init frames name where the DDS participant was created; they do not prove ownership of the conflicting memory. Both installed DDS and crypto plugins have zero __tsan_ symbol references, verified with readelf. The [primary TSan manual](https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual#non-instrumented-code) explains that uninstrumented libraries can cause misleading reports or missed synchronization. This does NOT establish that these particular warnings are false. Do not suppress them and claim a clean full program. Rebuild/inspect the matching DDS dependency with instrumentation to investigate.

### Valgrind actual-game failure, with zero leaked heap at exit

Command:
```bash
python3 tests/p4_valgrind.py --security-dir /tmp/dds-asteroids.7unxZ8xi
```
Harness exited1 at `Valgrind client failure`; the clients' error-exit code was nonzero. Server and all five clients participated and freed all their heap blocks. Root read each raw Memcheck file:
```text
client1-memcheck.log heap= 0 bytes in 0 blocks errors= 61 errors from 1 contexts (suppressed: 0 from 0)
client2-memcheck.log heap= 0 bytes in 0 blocks errors= 62 errors from 1 contexts (suppressed: 0 from 0)
client3-memcheck.log heap= 0 bytes in 0 blocks errors= 62 errors from 1 contexts (suppressed: 0 from 0)
client4-memcheck.log heap= 0 bytes in 0 blocks errors= 63 errors from 1 contexts (suppressed: 0 from 0)
client5-memcheck.log heap= 0 bytes in 0 blocks errors= 62 errors from 1 contexts (suppressed: 0 from 0)
server-memcheck.log heap= 0 bytes in 0 blocks errors= 0 errors from 0 contexts (suppressed: 0 from 0)
```
Actual client diagnostic:
```text
Syscall param sendmsg(msg.msg_iov[0]) points to uninitialised byte(s)
Uninitialised value was created by a stack allocation
```
The call/taint path includes libcrypto.so.3, libdds_security_crypto.so, libddsc.so.0.10.4 then application dds_write/send_flood. This may involve native crypto/serialization bytes; it remains unresolved and is not silently dismissed as harmless padding. No suppressions were used. Raw outputs: build/p4-valgrind/*-memcheck.log. The leak portion passes, but the overall Memcheck acceptance FAILS.

### Snapshot confidentiality observation

Before quota, tester ran tests/p3_snapshot_crypto.py: five real clients,169 live server snapshots,828 serialized outgoing bytes recorded unchanged (before==after), transformationskind4=AES256GCM confirmed against installed types, mutations0. Server output:
```text
SERVER_SUMMARY joins=5 rejects=0 leaves=5 timeouts=0 invalid=0 stale=0 peak=5 inputs=360 rate_hits=0 auth_failures=0 middleware_events=0 moves=2199 shots=185 destroyed=36 splits=19 hits=0 respawns=0 gameovers=0 restarts=0 wraps=5 snapshots=169 points=2380 ticks=515
WIRE_FAULT_SUMMARY calls=1268 mutations=0
NATIVE_GCM_SUMMARY failures=0
```
Artifacts: build/p3-snapshot-crypto/server-egress-before.bin and after.bin (828bytes each). This observes real protected server socket egress during a Snapshot workload. Opaque ciphertext is not claimed to reveal a decoded topic, and this is not a privileged external packet capture. External capture permission remains denied.

### Fresh source tree, final code

After the audit time-guard change, root repeated fresh public source-copy verification: `/tmp/dds-clean-source.butdc2m7`, `make -j2 all test check-hardening`, exit0 warning-free, all behavior tests and both hardening checks pass. Full raw output is build/clean-copy-verification.log. This is a fresh source-copy build, not a literal committed clean clone; commits remain human-owned.

### Current gate and exact human prerequisite

Ubuntu has neither cmake nor ninja; curl/git are present. `which cmake` returns1/no path. Building an isolated instrumented matching Cyclone DDS for the unresolved findings needs CMake. Per CODEX_GOAL.md missing-tool rule agents stop rather than sudo/install. Human command inside Ubuntu:
```bash
sudo apt update
sudo apt install -y cmake
```
This enables investigation; it is not a guarantee that dependency warnings disappear. P4 is INCOMPLETE. P5/P6, ten-minute soak, literal clean clone, two-machine play and human feel/packet checks remain pending. The local playable game, release/GUI/security/physics results and ASan/hardening passes are preserved separately from this failure. No entire-definition-of-done claim.


## Fresh local main-integration verification

See INTEGRATION.md for exact evidence paths. Independent builder release/unit/baseline/hardening returned0 (baseline warnings and initialclockskew disclosed). Initial independentP1 run1 failed (spoofclient admissiontimeout and server shutdownhang); retained. Run2 failedclosed because temporarycredential directory was absent; retained. PersistentLinuxhome fixture generated privately. Sequentialfallback unitregression passed existingfile/symlink preservation. Actualrun3 output below, observedexit0; this does not explain or clear the earlierhang or any P4 finding.

```text
COMMAND python3 -B tests/p1_network.py --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --output-dir build/evidence/integration-p1-run3
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/server --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --duration 180 --audit-file /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/evidence/integration-p1-run3/server-audit.log
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/client --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --identity 1 --headless --bot circle --duration 120
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/client --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --identity 2 --headless --bot circle --duration 120
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/client --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --identity 3 --headless --bot circle --duration 120
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/client --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --identity 4 --headless --bot circle --duration 120
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/client --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --identity 5 --headless --bot circle --duration 120
PASS concurrent joins: five distinct nonces and slots
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/client --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --identity 6 --headless --bot circle --duration 3
PASS sixth client rejected
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/client --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --identity 6 --headless --bot circle --duration 120
PASS explicit leave permits replacement
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/client --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --identity 2 --headless --bot circle --duration 120
PASS SIGKILL lease expiry permits rejoin
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/client --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --identity 6 --headless --bot circle --duration 30 --fault spoof
PASS SERVER_DROP identity=6 reason=spoof
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/client --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --identity 6 --headless --bot circle --duration 30 --fault session
PASS SERVER_DROP identity=6 reason=session
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/client --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --identity 6 --headless --bot circle --duration 30 --fault id-zero
PASS SERVER_DROP identity=6 reason=id-zero
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/client --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --identity 6 --headless --bot circle --duration 30 --fault id-high
PASS SERVER_DROP identity=6 reason=id-high
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/client --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --identity 6 --headless --bot circle --duration 30 --fault replay
PASS SERVER_DROP identity=6 reason=replay
SERVER_SUMMARY joins=12 rejects=1 leaves=11 timeouts=1 invalid=4 stale=1 peak=5 inputs=316 rate_hits=0 auth_failures=0 middleware_events=21 moves=2454 shots=206 destroyed=52 splits=25 hits=0 respawns=0 gameovers=0 restarts=0 wraps=0 snapshots=185 points=3680 ticks=700
PASS lifecycle audit records=36 root=f2875a5a707dde32868dbe318b15f0859f66a68f05f361bb4e6cb22a98a06dfa
PASS P1 networking lifecycle and malformed-message counters
OBSERVED_EXIT=0

```


## Current blocker: real secure GUI acceptance failed

Actual `xvfb-run ... tests/p3_gui.py --security-dir <persistent Linux bundle> --output-dir build/evidence/integration-gui-run1` exited1. Both windows rendered, but the server summary had joins2, peak1, timeouts2, inputs5, moves0, shots0, snapshots40, ticks1500. The first lease expired before the second player joined. Both final PNGs show WAITING FOR GAME STATE rather than a current connected two-player world. Root inspected both actual images. This contradicts accepting the merged game as working locally on this host, regardless of contributor historical reports. It is not cleared by the headless run3 pass.

Evidence: build/evidence/integration-gui-output.txt, integration-gui-exit.txt, integration-gui-run1/*.log and client1-10s.png/client2-10s.png. Cleanup completed; no successful manual GUI session is currently left open. Do not fabricate a manual-phase pass or open the game as if all own checks passed.

The current bounded integration attempt cycle is stopped at the original maximum-three-attempt rule. The middle missing-/tmp-bundle prerequisite failure is disclosed, not silently erased to extend the limit. Request a human decision to authorize a new targeted GUI/network starvation/hang investigation cycle. No fourth network attempt or second GUI attempt has been run. A diagnosis is not established: do not label native dependency, GPU, clock, or the harness as the cause without stronger evidence.

The merged implementation and all local work remain preserved. P1 integration acceptance is INCOMPLETE despite passing headless functionality. Native-security and P3 source exist; P2/P3 have not been newly accepted here. P4 OPEN HIGH TSan/Valgrind findings, full soak, external capture, two-machine verification, P5 measurements and P6 deliverables remain required by the original scope/PDF checklist.

Actual GUI output (observedexit1):

```text
COMMAND xvfb-run -a -s -screen 0 1620x650x24 python3 -B tests/p3_gui.py --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --output-dir build/evidence/integration-gui-run1
SCREENSHOT build/evidence/integration-gui-run1/client1-before.png
SCREENSHOT build/evidence/integration-gui-run1/client2-before.png
SCREENSHOT build/evidence/integration-gui-run1/client1-after-input.png
SCREENSHOT build/evidence/integration-gui-run1/client2-after-input.png
SCREENSHOT build/evidence/integration-gui-run1/client1-10s.png
SCREENSHOT build/evidence/integration-gui-run1/client2-10s.png
Traceback (most recent call last):
  File "/mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/tests/p3_gui.py", line 124, in <module>
    main()
  File "/mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/tests/p3_gui.py", line 111, in main
    exercise(out, server, log, clients, windows)
  File "/mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/tests/p3_gui.py", line 71, in exercise
    assert counts.get(key, -1) >= minimum, f"GUI did not exercise {key}: {counts}"
           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
AssertionError: GUI did not exercise peak: {'joins': 2, 'rejects': 0, 'leaves': 0, 'timeouts': 2, 'invalid': 15, 'stale': 0, 'peak': 1, 'inputs': 5, 'rate_hits': 0, 'auth_failures': 0, 'middleware_events': 4, 'moves': 0, 'shots': 0, 'destroyed': 0, 'splits': 0, 'hits': 1, 'respawns': 1, 'gameovers': 0, 'restarts': 0, 'wraps': 0, 'snapshots': 40, 'points': 0, 'ticks': 1500}
OBSERVED_EXIT=1

```
Actual managed-launcher check output (headless only):

```text
COMMAND bash tools/play.sh --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --headless --duration 600
make: Nothing to be done for 'all'.
Local session started. Logs: build/local-session.RWwE3GK5 server=429 client1=441 client2=553
SECURE_SESSION_READY; confirm actual windows separately.
OBSERVED_EXIT=0
COMMAND bash tools/play.sh --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --headless --duration 600
Secure session already running; use tools/stop.sh
OBSERVED_EXIT=1
COMMAND bash tools/stop.sh
Stopped owned secure session.
OBSERVED_EXIT=0
Post-stop exact process query: no game process rows.

```
Fallback unit target returned0 and printed PASS audit exclusive creation and symlink target preservation; all existing protocol/sim fixtures passed. This establishes the audit fix, not resolution of runtime GUI stalls.


## Paused teammate checkpoint
User explicitly paused goal dueusage and authorized handover+commit/push. HANDOVER.md is current; selected actual nonsecret output copied under docs/evidence/2026-11-04-handover/ because build/ is ignored. Native ACK diagnostic measured requested1e9ns elapsed1000082330ns result-10; this is not a waitset overshoot. Latest active GUI has timeouts0/invalid0/freshconnected state, but overall network and GUI remain FAIL during ACK shutdown. No game tests/repairs ran afterpause, no private material included, no completion claim.


## 5 November 2026 - isolated ACK versus unchanged P1 churn

Scope: independent tester resumed with explicitly authorized bounded diagnostics.
The production one-second ACK request, three-second application lease, native
security, IDL and acceptance assertions were not changed for either case. The
shim is diagnostic-only and is loaded only into DDS-linked server/client children.
Neither an instrumented pass nor application Leave receipt establishes full P1/P4
acceptance. No sudo or private-key contents were read or printed.

### Server lifetime hypothesis: source and recorded chronology

The full P1 harness (`tests/p1_network.py:81`) starts its server with duration180s.
It stops the fault client at line115 before the normal server stop at line120;
failure cleanup at line137 signals owned children in reverse creation order, so
the server is last. `src/server/main.c` derives a10800-tick budget at60Hz after
readiness. The earlier ACK run ended at598ticks, and the fresh comparison at540,
far below that budget. There is no evidence that the server's configured lifetime
expired before ACK. Application receipt is separate from native reliable-reader
acknowledgement.

Earlier ACK record: auditSTART1793790689297406767ns; client ACKentry realtime
1793790697062774957ns (7.765368190s later); server Leave recorded
1793790697063665865ns (+0.890908ms after ACKentry); native ACK timeout about1s
after entry. Other legitimate client leaves were recorded only after+2073.679ms.
These are recorded timestamps, not an invented emission or network-packet trace.

### Isolated case: PASS for this bounded diagnostic only

One fresh identity6 spoof client, no peer churn, server60s/client30s. The probe
waited for accepted assignment, actual spoof rejection and an own-state snapshot,
requested clientSIGTERM, then retained the server through client exit plus2s.
Actual probe command, child commands and result follow verbatim:

```text
COMMAND python3 -B tests/ack_probe.py --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --dds-markers /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/dds_call_markers.so --output-dir build/evidence/ack-resume-isolated1
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/server --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --duration 60 --audit-file /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/evidence/ack-resume-isolated1/audit.log
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/client --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --identity 6 --headless --bot idle --fault spoof --duration 30
{
  "scope": "isolated single-identity Leave/ACK diagnostic",
  "server_duration_seconds": 60,
  "client_duration_seconds": 30,
  "production_ack_timeout_seconds": 1,
  "events": [
    {
      "event": "child_started",
      "role": "server",
      "pid": 2171,
      "monotonic_ns": 10602460158622
    },
    {
      "event": "server_ready_observed",
      "monotonic_ns": 10602572519661
    },
    {
      "event": "child_started",
      "role": "client",
      "pid": 2179,
      "monotonic_ns": 10602576133197
    },
    {
      "event": "client_sigterm_requested",
      "monotonic_ns": 10603369050441,
      "realtime_ns": 1793817955796372876,
      "server_alive": true
    },
    {
      "event": "client_exit_observed",
      "monotonic_ns": 10603435506220,
      "realtime_ns": 1793817955862829029,
      "code": 0,
      "server_alive": true
    },
    {
      "event": "cleanup_signal",
      "pid": 2171,
      "monotonic_ns": 10605460764937
    }
  ],
  "assigned_id": 1,
  "client_exit": 0,
  "server_alive_after_client_exit": true,
  "server_alive_two_seconds_later": true,
  "server_leave_observed": true,
  "audit_leave_observed": true,
  "client_cleanup_exit": 0,
  "server_cleanup_exit": 0,
  "server_summary_observed": true,
  "status": "PASS"
}
OBSERVED_EXIT=0

```

Actual isolated client output (public addressing and numeric timing only):

```text
CLIENT_JOIN nonce=38ea7446b82ab3c59ad05a62ddbf8e6b id=1 status=accepted
CLIENT_SNAPSHOT tick=42 id=1 x=18000 y=30000 score=0 lives=3 vx=0 vy=0 distance_mm=0
DDS_ACK_MATCH stage=before result=1 query_elapsed_ns=4911 clock_valid=1
DDS_ACK_RESULT returned=0 requested_ns=1000000000 elapsed_ns=1843 clock_valid=1
DDS_ACK_MATCH stage=after result=1 query_elapsed_ns=1279 clock_valid=1
CLIENT_EXIT

```

Actual isolated server output:

```text
AUDIT_FILE /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/evidence/ack-resume-isolated1/audit.log
SERVER_READY security=NATIVE_DDS domain=42
SERVER_JOIN identity=6 id=1 status=accepted
SERVER_DROP identity=6 reason=spoof
SERVER_LEAVE identity=6 id=1
SERVER_SUMMARY joins=1 rejects=0 leaves=1 timeouts=0 invalid=1 stale=0 peak=1 inputs=0 rate_hits=0 auth_failures=0 middleware_events=0 moves=0 shots=0 destroyed=0 splits=0 hits=0 respawns=0 gameovers=0 restarts=0 wraps=0 snapshots=57 points=0 ticks=176
AUDIT_ROOT hash=0ce7eaf86ec8a88e333d099a57d38fa355da8cb2053c900481b8ccc7b35d4b9d records=4

```

Matched-subscription query returned1 before and after. Native ACK returned0 in
1843ns for the unchanged1000000000ns request. This is a subscription count, not
an identity or per-reader acknowledgement report. Client/server exit0, Leave
receipt/audit and live server two seconds after client exit were observed.

### Unchanged full P1 churn comparison: FAIL, exit1

The existing acceptance suite was run once with the same compiled diagnostic shim.
Concurrent admission/cap/leave/crash recovery reached their existing assertions.
It again failed on the spoof client's nonzero shutdown, without relaxing that
assertion. Actual command and output:

```text
COMMAND python3 -B tests/gui_runtime_diagnostics.py --suite network --dds-markers /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/dds_call_markers.so --diagnostics build/evidence/ack-resume-network1.jsonl --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --output-dir build/evidence/ack-resume-network1
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/server --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --duration 180 --audit-file /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/evidence/ack-resume-network1/server-audit.log
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/client --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --identity 1 --headless --bot circle --duration 120
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/client --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --identity 2 --headless --bot circle --duration 120
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/client --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --identity 3 --headless --bot circle --duration 120
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/client --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --identity 4 --headless --bot circle --duration 120
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/client --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --identity 5 --headless --bot circle --duration 120
PASS concurrent joins: five distinct nonces and slots
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/client --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --identity 6 --headless --bot circle --duration 3
PASS sixth client rejected
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/client --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --identity 6 --headless --bot circle --duration 120
PASS explicit leave permits replacement
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/client --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --identity 2 --headless --bot circle --duration 120
PASS SIGKILL lease expiry permits rejoin
COMMAND /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/release/client --security-dir /home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01 --identity 6 --headless --bot circle --duration 30 --fault spoof
Traceback (most recent call last):
  File "/mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/tests/gui_runtime_diagnostics.py", line 142, in <module>
    raise SystemExit(main())
                     ^^^^^^
  File "/mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/tests/gui_runtime_diagnostics.py", line 128, in main
    return acceptance.main()
           ^^^^^^^^^^^^^^^^^
  File "/mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/tests/p1_network.py", line 115, in main
    assert stop(proc) == 0, f"Fault client {fault} shutdown failed"
           ^^^^^^^^^^^^^^^
AssertionError: Fault client spoof shutdown failed
OBSERVED_EXIT=1

```

Actual failing client output:

```text
CLIENT_JOIN nonce=b0e79f31748542f8b093b68123a76db1 id=2 status=accepted
CLIENT_SNAPSHOT tick=461 id=2 x=29000 y=30000 score=0 lives=3 vx=0 vy=0 distance_mm=0
DDS_ACK_MATCH stage=before result=1 query_elapsed_ns=5549 clock_valid=1
DDS_ACK_RESULT returned=-10 requested_ns=1000000000 elapsed_ns=1000571819 clock_valid=1
DDS_SLOW_CALL op=acks elapsed_ns=1000571819 requested_ns=1000000000 returned=-10 before_dds_ns=1793818034401768788 after_dds_ns=1793818035402341248 before_rt_ns=1793818034401766247 after_rt_ns=1793818035402340960 delta_dds_ns=1000572460 delta_rt_ns=1000574713 clock_valid=1
DDS_ACK_MATCH stage=after result=1 query_elapsed_ns=21354 clock_valid=1
leave_ack: Timeout
CLIENT_EXIT

```

Before/after matched-subscription counts were1 for this failing writer too.
Native ACK returned-10 after1000571819ns for a1000000000ns request; DDS and
realtime deltas during that call were approximately1s and clock_valid=1.
Other observed client ACKs in this comparison returned0 in roughly1.8-4us.
The failure is not explained by a proven additional matched-reader count or
by the older4.75s/50ms native waitset overshoot.

Actual comparison server tail:

```text
SERVER_JOIN identity=2 id=3 status=accepted
SERVER_LEAVE identity=2 id=3
SERVER_LEAVE identity=6 id=2
DDS_DIAGNOSTIC category=128 reason=decode function=q_omg_security_decode_rtps_message cause=classified
SERVER_JOIN identity=6 id=2 status=accepted
SERVER_DROP identity=6 reason=spoof
SERVER_LEAVE identity=6 id=2
SERVER_LEAVE identity=5 id=5
SERVER_LEAVE identity=4 id=4
SERVER_LEAVE identity=3 id=1
DDS_SLOW_CALL op=delete elapsed_ns=1026692248
SERVER_SUMMARY joins=8 rejects=1 leaves=7 timeouts=1 invalid=1 stale=0 peak=5 inputs=283 rate_hits=0 auth_failures=0 middleware_events=13 moves=1939 shots=164 destroyed=28 splits=12 hits=0 respawns=0 gameovers=0 restarts=0 wraps=0 snapshots=173 points=2080 ticks=540
AUDIT_ROOT hash=9cb31423ed973b1be1c2fa4327fd84f9f6776ad05f15a6af3712a4de48c0cd54 records=21

```

### New clock observation: cause not yet established

The fresh comparison audit has increasing hash-chain sequence numbers but a
decreasing realtime reading across sequence16to17. Selected actual lines:

```text
seq=16 ns=1793818035774977702 identity=6 event=JOIN reason=admission previous=988cfeb8f737aef88d15d112717c35173a9eecd10c0e92d762daae909b64b9b8 hash=6f9e7f91b71842a77af18c2b647a90c26c54ef39dfef9341e6e1794d9cde1604
seq=17 ns=1793818034347470698 identity=6 event=DROP reason=spoof previous=6f9e7f91b71842a77af18c2b647a90c26c54ef39dfef9341e6e1794d9cde1604 hash=35ef728f9300f8cc787bc63c98b3db4c2a06ada118d860bb821135adcc1a2ef6
seq=18 ns=1793818034401339626 identity=6 event=LEAVE reason=explicit previous=35ef728f9300f8cc787bc63c98b3db4c2a06ada118d860bb821135adcc1a2ef6 hash=870e3eba8bb5cf6268fda67b064c8862a76c15bb0faf4cd3b47196c1a9766368

```

1793818035774977702ns to1793818034347470698ns is a backward difference of
1427507004ns (1.427507004s). `audit_log.c` reads CLOCK_REALTIME under the audit
mutex before serializing each sequence. This is an observed nonmonotonic walltime
record, not proof of its underlying cause or that it caused the ACK timeout.
Clocks agreeing during the later one-second ACK call does not rule out an earlier
clock discontinuity. No clock was changed or timestamp clamped by the tester.
A paired-clock observation during a controlled case needs separate authorization;
it has not been run as part of this report.

### Cleanup and honest acceptance status

After each case, the read-only command
`ps -C server -C client -o pid=,stat=,comm=` returned no process rows (exit1).
No game process was left live at those observations. No GUI test, full-suite
repetition or production repair was performed after the comparison failure.

Isolated case: observed exit0, diagnostic PASS. Full P1 comparison: observed
exit1, FAIL. P1/GUI acceptance remains incomplete; all prior P4 HIGH findings and
submission requirements remain open. Full raw outputs, per-child logs, summary
and read-only diagnostics remain under ignored `build/evidence/ack-resume-*`.
The selected exact output above is retained in this tracked report so evidence
is not lost merely because build artifacts are ignored.


## Approved T1 fallback verification - 5 November 2026

User approval: "Approved T1". Separate review confirmed only DDS_RETCODE_TIMEOUT following successful Leave write is downgraded to an explicit warning. Write errors, other ACK errors and cleanup errors remain fatal. No security, QoS, lease, wire-format or timeout changes. The native ACK problem remains a known limitation.

Actual incremental client compile exit0; full compiler output: build/evidence/slim-t1-build.txt. Targeted native lifecycle run, no preload/thread cap: tests/p1_network.py --security-dir <private Linux bundle> --output-dir build/evidence/slim-t1-network1; exit0. Output includes PASS fresh server slot release per identity, PASS final fault slot permits replacement, SERVER_SUMMARY joins13 rejects1 leaves12 timeouts1 invalid4 stale1 peak5, and PASS lifecycle audit records36. One timeout is the intentional SIGKILL lease test. Complete output and exit file retained; this run did not emit the warning and alone did not exercise the fallback branch.

Explicit test-only public API fixture tests/force_ack_timeout.c simulates DDS_RETCODE_TIMEOUT, without probing internals. Compiled with cc -std=gnu11 -Wall -Wextra -Werror -fPIC -shared; isolated tests/ack_probe.py --dds-markers build/force_ack_timeout.so --output-dir build/evidence/slim-t1-forced-timeout exited0. Actual output: CLIENT_WARNING leave_ack_timeout: transport acknowledgement unconfirmed; server slot release requires independent verification; CLIENT_EXIT; server_leave_observed true; audit_leave_observed true; client_cleanup_exit0; server_cleanup_exit0; status PASS. This establishes application handling of an injected timeout, not native ACK reliability. Full output: build/evidence/slim-t1-forced-timeout-output.txt.

T1 approved fallback implemented and targeted checks passed; T2 reproduction pending. Existing unsuppressed dependency findings and all historical failures remain disclosed.


## T2/T3 slim checkpoint - 5 November 2026

T2 fresh clone plus current working patch: build/unit/hardening exit0; headless124.539seconds after both admissions, joins2/leaves2/timeouts0/invalid0, audit verified; strict GUI exit0, both new final screenshots inspected once with connected HUDs, ships and asteroids. Not a clean published final revision. Complete nonsecret output copied into docs/evidence/2026-11-05-slim-finish/. T3 docs/EVIDENCE.md uses existing results only, labels historical P2 results, records missing idle CPU/profiler data and all excluded work as limitations. Exact course perf commands prepared with explicit missing-binary prerequisite. STOP at T3 approval gate; no T4 capture work or T5 packaging yet. Native ACK reliability remains unproven despite approved application fallback. No new sanitizer runs or benchmarks.


## T3 approved; explicit manual guide and capture preparation - 5 November 2026

User: "approved, but with a catch. show me how to start the game, with explicit instructions for me to test and verify. tell me exactly what you did , instructions to build and run the program." Read current launch scripts and actual key bindings. Added RUN_AND_VERIFY.md with exact Ubuntu build, credential, server/client terminal commands, observable human checklist, targeted lifecycle reproduction, managed shortcut/stop distinction and source-ZIP instructions. README stale failure-only banners replaced with current evidence and limitations; no application changes this turn.

T4 one targeted capture-helper update: old P0 snaplen256/count10000 replaced by snaplen65535/count4000, accurate secure-game guidance; original ownership/mode/no-overwrite safeguards preserved. Offline tests: PASS bash syntax; PASS invalid duration/path rejected before capture; PASS offline IPv4 RTPS filter: 1 matching packet, complete2042bytes, non-RTPS excluded; exit0. This is synthetic fixture evidence only, not encryption or privileged capture evidence. Full output docs/evidence/2026-11-05-slim-finish/slim-t4-capture-check.txt. Human live capture and subsequent unprivileged analysis remain required. No full suite, benchmark, sanitizer or GUI rerun this turn.


## User-requested automatic launcher and lecturer checklist test - 5 November 2026

See REQUIREMENTS_CHECK.md and docs/evidence/2026-11-05-requirements/. Real play_local.sh --duration45 under isolated software-GL Xvfb passed two admissions/windows, real key input, fresh shared state, first-client Leave and session exit0: joins2 leaves2 timeouts0 invalid0 peak2 moves1627 shots14 snapshots310. Both new final screenshots inspected once. Actual WSL and OpenGL/GLFW/DDS linkage confirmed; no remaining game/Xvfb process rows.

Targeted authentication first attempt FAILED due to my identity2 fixture selection: rogue_client2.crt missing, so no server auth-warning audit. Preserved and reported. One test-setup correction selected generated rogue identity6: wrong-ca and no-cert rejected exit1 while a legitimate client stayed alive; no extra slots, auth_failures6, server audit verified, test exit0. No product/security/assertion change.

Approval is limited to tested launcher/local multiplayer/certificate rejection. Cross-machine operation and external encryption capture remain unverified; cached-glyph/condition-wait concepts are confirmed but speedup/idleCPU is unmeasured. Known ACK/sanitizer limitations retained. No full-suite or sanitizer/benchmark rerun and no agent sudo. User's full requirement list is not yet approved.

## Account integration,2026-11-05
Evidence archive sibling: Networking_CSD3116_integration_20261105_c50af1e. account-reconnect-build.txt: BUILD_EXIT0. account-reconnect-test-02.txt: TEST_EXIT0, authenticated crash/rejoin keeps player1; second absence ends entire round; server/survivor exit0, elapsed31.981s including final display. c1/restored snapshot logs: score260->660, lives3, distance166663->306794. account-final-regression.txt: REGRESSION_EXIT0, protocol/sim/event/account-store units and PIE/NX/RELRO/BIND_NOW/canary checks; certificate-only rejection, forged-session/unknown-account/rapid-retry rejection, earned score420 persisted, leaderboard/token rotation/replay/binding/logout/forget passed. Countdown field31 rejected;30 accepted (countdown-boundary-test.txt exit0). Final integrated offline package verification pending.

## Final integrated offline result
# Final runtime integration,2026-11-05

The selected account/password/SQLite implementation is integrated with the30second crash/rejoin reservation. The verified runtime is now in Networking_CSD3116; every previous root file except preserved .git was moved into root-before-account-promotion in this archive. Full development source, tests and docs remain in Networking_CSD3116_accounts_c50af1e. The tested ZIP contains runtime source, launch/build/provision scripts, generated DDS source, public dependency headers/libraries, required license records and RUN_OFFLINE.md. No private keys, profiles, databases, captures, logs or Git history are in the ZIP.

Ubuntu24.04 x86_64 under WSL2/WSLg with an existing GCC C toolchain is required. Run bash run.sh from the extracted DDS_Asteroids directory. No packages or downloads are invoked. Build-only: bash tools/build_portable.sh.

Actual final outputs (offline-final-verification.txt, exit0):
PASS bundled Python modules sqlite3/ssl/ctypes/fcntl/json
PASS fresh ZIP extraction: offline compile, fresh certificate provisioning, authenticated two-client game and exit0
PASS fresh ZIP bundled software OpenGL: two visible GLFW windows, registration, authenticated joins, keyboard input, screenshot, exit0
PASS offline extracted two-minute two-client soak: exit0 joins2 timeouts0 invalid0

The fresh-extraction test blocked apt,apt-get,sudo,make,pkg-config,idlc. New screenshot gui-offline.png inspected once: both players connected, separate player1/player2 HUD, asteroids, ships, firing and distinct account names.

Native account regression and hardening: account-final-regression.txt, exit0. PIE/NX/RELRO/BIND_NOW/canary references for both binaries; protocol/sim/event/account-store unit checks pass. Certificate-only admission, forged account session, unknown account and rapid retry rejected. Server-earned score420 persisted. Leaderboard, remembered-token rotation/replay rejection/certificate binding/logout/forget all passed.

Reconnect: account-reconnect-test-02.txt exit0. Same-account/certificate crash reclaim kept player1. Score260->660, lives3, cumulative distance166663->306794. On second crash, entire round/server/surviving client ended with exit0 after31.981seconds including final display. Protocol version4 prevents silent compatibility with older game messages. Countdown boundary30 accepted/31 rejected (countdown-boundary-test.txt exit0).

Known limitations remain: external packet capture missing (previous human capture had0packets), no two-machine run, no controlled idleCPU before/after or full before/after performance measurements. Historical TSan/Valgrind dependency findings remain open, not rerun or cleared. Leave ACK warning fallback remains previously approved. During the terminal round-end interval, already in-flight inputs can produce session-drop audit records. WSLg desktop human acceptance has not been substituted by Xvfb automation. Slides/video/peer evaluation remain team deliverables. These are not claims of full rubric completion.

No new commit or push was performed. The final ZIP is local, not a published clean Git revision.


## Windows double-click launcher fix
The reported failure was opening run.sh directly from Windows File Explorer. The working-folder offline build and full run.sh headless session both exited0. Added RunGame.cmd, which explicitly invokes the default Ubuntu WSL distribution in the script directory and pauses after errors. Added visible server/client compilation progress. No C, IDL, account, security or SDK library changes.

Actual Windows entry-point test: RunGame.cmd --headless --duration8, WINDOWS_LAUNCH_EXIT=0:
Starting DDS Asteroids in WSL. The first compilation can take about a minute.
[1/2] Compiling server...
[2/2] Compiling client...
Offline source build complete. No packages installed.
Local session started. Logs: build/local-session.DJjRorpS server=9485 client1=9500 client2=9527

Updated distribution: DDS_Asteroids_offline_source_windows_launcher.zip,94314441bytes,1102 unique files, SHA2563060b8f082fcf858cbcec252e69e8374c898bfebe918700825eb73a926451484. Every packaged file verified against current runtime. No private keys/databases/profiles/captures included. The old ZIP is retained as historical evidence; distribute the Windows-launcher ZIP. Windows usage: extract the entire ZIP and double-click RunGame.cmd, not run.sh. Ubuntu terminal usage remains bash run.sh. Test mode above is headless; the earlier graphical registration/input evidence remains in offline-final-verification.txt. External packet-capture evidence remains pending.

## Password minimum and clearer wording,2026-11-05
User requested minimum6letters and simpler wording. Passwords now accept6-128 printable characters with no uppercase/number/symbol requirement. Both network/client validation and the server store enforce the same lower bound. Existing password hashes, accounts, tokens, database schema, TLS/DDS security and wire format unchanged.

Changed account form to Sign in or create an account, Create account and Stay signed in. Removed internal jargon about certificates, authentication and password bytes from the login form. Field-specific username/password format errors and plain generic sign-in failure messages preserve account-enumeration protection.

Actual outputs:
PASSWORD_TEST_EXIT=0
PASS password minimum: five rejected, six/seven letters accepted
PASS six-letter password registration/login; five-letter password rejected
PASS existing store score/token/binding/path safety regressions
PASSWORD_BUILD_EXIT=0 (bundled offline build, no packages installed)
PASSWORD_GUI_EXIT=0
PASS GUI: five-letter password blocked; adding sixth letter registers player1; six-letter player2 registration accepted
PASS both graphical clients joined; password text remains masked; screenshots saved for visual inspection

password-six-validation.png and password-six-connected.png inspected once: clear, fitting form text, five masked password characters and format error, second player's six-character hint; final screenshot both authenticated players in game. No full-suite/soak or sanitizer rerun. Distribute DDS_Asteroids_offline_source_password6.zip; it retains Windows and reconnect launchers. Restart with RunGame.cmd to load the change. External packet capture remains pending.

## Multiple accounts per player,2026-11-05
User explicitly approved allowing any number of registered accounts per player window. Removed the per-identity unique constraint and global six-account capacity trigger. Usernames remain globally unique. Accounts/tokens stay certificate-identity bound; one active client per participant identity and the gameplay player cap remain unchanged. Passwords still allow six digits with no composition requirement. Registration errors now suggest a different username, not an account-per-window limit.

Schema version2 is created for new stores. Existing version1 stores are validated under the exclusive lock and upgraded transactionally: create/copy the accounts table preserving all IDs/columns, remove old restrictions, validate integrity/foreign keys, commit or roll back, restore foreign-key enforcement. Password hashing, existing password values, score provenance and tokens are preserved.

MULTI_STORE_EXIT=0 (multi-account-store-test.txt):
PASS legacy migration preserves every account/score/token field byte-for-byte
PASS twelve accounts on one player beyond old global cap; login/reopen/unique usernames and certificate binding
PASS existing six-character, score persistence, verifier material, token rotation/expiry/revocation, unsafe-path and exclusive-lock checks
MULTI_BUILD_EXIT=0 (multi-account-build.txt): bundled offline build of client/server in development worktree; no packages installed.
REAL_MIGRATION_EXIT=0 (real-account-migration-test.txt): migrated a private copy of the user's exact original-schema database1->2; both accounts and every stored account/password/score/token value identical; original and backup untouched.

Private consistent backup: /home/stanley/.local/state/dds-asteroids/backups/accounts-before-multi-3y42hbou.sqlite3 (mode0600 under mode0700 backups). No private data is included in the source ZIP. The live user's old server was left running to protect their session; the primary runtime contains updated source. Close the old launcher terminal, then double-click RunGame.cmd to compile/restart and upgrade the live store. Do not run the older game against the upgraded schema. Historical artifacts are retained, not the current distribution.

Distribute DDS_Asteroids_offline_source_multi_accounts.zip. No new DDS wire/security mechanism change, full-suite/soak/sanitizer rerun or live-session interference. Account registration/login and migration were verified through actual store APIs; no new concurrent DDS network test was run alongside the user's live server. External packet capture remains pending.
