> **Current slim checkpoint, 5 November 2026:** [Exact build/run commands and human checklist](docs/RUN_AND_VERIFY.md). [Evidence and limitations](docs/EVIDENCE.md). T1 timeout-only fallback approved and verified; fresh clone plus uncommitted patch passed build/units/hardening,124.539-second two-client headless and one strict GUI check. Native Leave ACK reliability is still unproven. Historical failures and unsuppressed dependency findings are retained. No submission-ready claim.

> **Current verification:** see [docs/CURRENT_REVIEW.md](docs/CURRENT_REVIEW.md). Fresh build/unit/hardening, secure five-player lifecycle and strict two-window GUI checks pass on Jordain's machine. The full negative-security suite also passes. Preserved ACK-timeout/server-hang failures on Stanley's host and P4 memory/thread findings remain unresolved. [docs/HANDOVER.md](docs/HANDOVER.md) retains the earlier host's evidence.

# DDS Asteroids

Secure, server-authoritative co-op Asteroids in C, using Cyclone DDS, OpenGL, GLFW and OpenSSL. CSD3116, DigiPen SIT. Author: <team>, with declared Codex assistance; updated 2026-11-04.

**Playable locally:** the latest strict two-window run includes fresh connected state, real movement/fire and normal shutdown. Historical ASan/UBSan churn passed; current listener changes are being reverified. Full memory/thread acceptance remains incomplete: DDS-library TSan reports and Valgrind uninitialised crypto-send bytes require investigation. LAN play, human feel, performance measurements and external packet inspection remain outstanding.

## Quick start on this Windows machine

Open Command Prompt or PowerShell in your own repository checkout. Replace the example path below with its actual location:

```cmd
cd C:\path\to\Networking_CSD3116
wsl -d Ubuntu -- bash tools/play_local.sh
```

Or, inside Ubuntu:

```bash
cd /path/to/Networking_CSD3116
bash tools/play_local.sh
```

The launcher builds the game, prepares private Linux credentials and opens two player login windows. Register a different username in each window on the first run. Enter 3–24 lowercase letters, digits or underscores, press Tab, then enter a password of 12–128 printable ASCII characters. Press Shift+Enter to register or Enter to sign in to an existing account. Use the same window identity on later runs. F2 opts into Remember Me before submitting. On the next launch, the client tries the remembered login automatically.

Click a window to control that player. Closing either window ends this local session. Each run prints its unique `build/local-session.*` log directory. Use `--duration 120` for two minutes or `--headless --duration 8` to run two bots with temporary test accounts. Keep using the same CA bundle to access saved accounts and scores. `--fresh-certs` creates a new CA and a separate account scope. Automated graphical evidence uses Xvfb and software OpenGL.

| Control | Action |
|---|---|
| Up | Thrust |
| Left / Right | Rotate |
| Space | Fire |
| R | Restart after shared game-over |
| B | Show or hide the saved leaderboard |
| L | Log out and revoke the remembered login |
| F3 | Forget the remembered login |
| Escape | Leave / close window |

Ships have inertia/friction and wrap across all edges. Large rocks split into medium, then small; kills award 20/50/100 points. Three lives per player; a hit causes a one-second respawn and two seconds of protection. Friendly fire is off. The HUD shows all players' lives/scores, wave, connection state and an input-echo RTT estimate. See `docs/P3_GAME.md` for mechanics, snapshot bounds and threading.

## Requirements and build

Ubuntu 24.04/WSL2, GCC/C11, make, pkg-config, Cyclone DDS development/tools, GLFW3, OpenGL, and OpenSSL and SQLite development headers. Graphical play needs WSLg or an X11 display. This machine uses Cyclone DDS 0.10.4, GLFW 3.3.10, OpenSSL 3.0.13 and SQLite 3.45.1.

A human can install dependencies inside Ubuntu:

```bash
sudo apt update
sudo apt install -y build-essential pkg-config git gdb valgrind cmake openssl libssl-dev libsqlite3-dev libgl1-mesa-dev libglfw3-dev cyclonedds-dev cyclonedds-tools xvfb xauth imagemagick xdotool tcpdump
```

From a fresh checkout of this repository:

```bash
make -j2
make test
make baseline
make SAN=asan
make SAN=tsan
make check-hardening
```

Game executables are `build/release/server` and `build/release/client`; sanitizer builds have separate directories. IDL bindings are generated automatically. Original lecturer bindings are isolated under `build/baseline-gen`; baseline/third_party reference files remain unchanged. The baseline has original source warnings; new game code builds warning-free. Build flags alone do not prove sanitizer cleanliness: see `docs/TEST_REPORT.md` for actual results and limitations.

## Credentials and separate processes

```bash
bash tools/gen_certs.sh
```

The final line prints `SECURITY_DIR=/tmp/dds-asteroids.<random>`. Use the actual path below. The Linux directory is 0700 and private keys are 0600; they may disappear on reboot. The Windows-mounted repository does not enforce these Unix modes, so credentials stay outside it. Normal game processes fail closed on unsafe/missing credentials, plugins or policies.

Generate a current signed bundle for the account topics and update both clients and the server together. For a reusable bundle, pass a new directory under your Linux home to `tools/gen_certs.sh`. The server saves accounts and best scores in `~/.local/state/dds-asteroids/server-<CA fingerprint>/accounts.sqlite3`. Opted-in client profiles live under `profiles-<CA fingerprint>` in the same state root. A best score reflects committed server results from completed participation, including leaving a round. Ordinary window close keeps a remembered login. Logout and online Forget revoke it. Treat databases and profiles as private runtime files.

```bash
./build/release/server --security-dir /tmp/YOUR_PRIVATE_BUNDLE --duration 3600
# Separate terminals, distinct identities:
./build/release/client --security-dir /tmp/YOUR_PRIVATE_BUNDLE --identity 1 --duration 3600
./build/release/client --security-dir /tmp/YOUR_PRIVATE_BUNDLE --identity 2 --duration 3600
```

Headless clients accept `--headless --bot idle|circle|straight|hunter` and use an existing remembered profile or `--credentials-fd N` with username and password on separate newline-terminated lines. `--register` selects registration for that input. Use the launcher for a self-contained two-bot run. The server also accepts `--ticks N` and the explicit test fixture `--test-scenario gameover`.

Native DDS Security uses mutual CA-chained X.509 identities, signed exact-topic permissions and encrypted governance. Only the server can publish authoritative snapshots/responses. The full provisioning fixture holds CA and all participant keys: **never distribute it**. Create a minimal bundle for each machine/participant:

```bash
bash tools/export_identity.sh /tmp/YOUR_PRIVATE_BUNDLE server
bash tools/export_identity.sh /tmp/YOUR_PRIVATE_BUNDLE client1
bash tools/check_secrets.sh
```

The secret checker can be installed by the human as a pre-commit hook. Agents do not commit or push. Never include private keys in source zips, logs, slides or videos. Preserve audit final root and record count separately; an unanchored hash chain does not prevent a complete log rewrite. Certificate/policy renewal and compromise limits are described in `docs/THREAT_MODEL.md`.

## Networking

`config/cyclonedds.xml` defaults to same-host unicast on loopback, domain 42, participant indices 0–15. It is automatically loaded unless `CYCLONEDDS_URI` is set. Standard DDS port arithmetic gives UDP 17910–17941 for these unicast participant pairs; packet capture can include 17900–17941. This is a local configuration, not an assertion that WSL NAT forwards multicast.

For two machines, configure each machine's actual LAN interface and explicit peer addresses in a separate XML and set `CYCLONEDDS_URI` to that file. Keep the trace/discovery categories if you need the native rejection/decrypt audit events. Share one trusted policy/CA provisioning set but give each process only its own minimal bundle. WSL mirrored/NAT behavior, firewall and real LAN latency are not verified by loopback tests. The [CycloneDDS configuration reference](https://cyclonedds.io/docs/cyclonedds/0.10.3/config.html) and installed headers are the authority for options.

## Reproduce checks

Replace the security directory with the actual generated fixture path. Run network harnesses one at a time: they share DDS domain 42.

```bash
make test
make build/release/account-join-probe
python3 -B tests/account_integration.py --security-dir /tmp/YOUR_PRIVATE_BUNDLE --output-dir build/accounts-check-01
python3 tests/p1_network.py --security-dir /tmp/YOUR_PRIVATE_BUNDLE
python3 tests/p3_gameplay.py --security-dir /tmp/YOUR_PRIVATE_BUNDLE
python3 tests/p3_gui.py --security-dir /tmp/YOUR_PRIVATE_BUNDLE
cc -shared -fPIC -Wall -Wextra tests/rtps_fault.c -o build/rtps_fault.so -ldl -pthread -lcrypto
python3 tests/p2_security.py --security-dir /tmp/YOUR_PRIVATE_BUNDLE
```

Security/P4 harnesses accept `--output-dir NEW_DIRECTORY` and refuse existing output so earlier failures survive reruns. Prefer private Linux-home storage for reusable credentials; `/tmp` fixtures can disappear between sessions.

For authorised external capture, start the secure game and have a human run this in a separate Ubuntu terminal (use a new basename each time):

```bash
sudo bash tools/capture_demo.sh --secure 30 secure-demo-01.pcap lo
```

Secure capture retains full IPv4 UDP packets, including later IP fragments, with a 2,000-packet/time bound. Other IPv4 UDP on the selected interface may be included. Inspect actual game packets and reassembly completeness offline before claiming encrypted-payload evidence. The default mode retains full unfragmented IPv4 RTPS packets, matching the upstream capture guide; `--baseline` explicitly selects truncated P0 reference capture. The agent never runs sudo; a prepared helper is not a completed capture.

The security harness uses genuine post-encryption ciphertext corruption, actual native GCM failure and separately chained production diagnostics; it does not substitute a plaintext change before encryption. No-cert discovery is logged as an unauthenticated observation with identity 0 under denying governance, not as an invented certificate failure. The forbidden server-publication test is denied locally by native access control; it does not invent a remote server event for a packet never sent.

Actual unit, five-bot, lifecycle, security, controlled game-over/restart and two-window results are in `docs/TEST_REPORT.md`; findings are in `docs/REVIEW_LOG.md`. Human checks of smoothness, controls, LAN play and Wireshark are in `docs/PLAYTEST.md`. Later phases cover sanitizers, Valgrind, measured optimisation and submission materials; their status is explicit in `docs/PROGRESS.md`.

## Remaining verification prerequisite

The playable release, unit/physics/security checks, five-client gameplay, controlled restart, two GUI windows, fail-closed hardening and ASan/UBSan churn have actual passing evidence. Full TSan and Valgrind acceptance **does not pass**. Disabling ASLR only for the TSan test processes fixes startup mapping, but the actual game still reports DDS-library races. Valgrind shows zero heap left at exit in all six processes but reports uninitialised bytes in client native-crypto sendmsg paths. No suppressions or blanket false-positive claims were used.

A matching instrumented dependency investigation needs CMake. Jordain installed it manually on 4 November; `cmake --version` reports 3.28.3. On other Ubuntu installations, a human can run:

```bash
sudo apt update
sudo apt install -y cmake
```

This is a prerequisite for investigation, not a promised fix. Agents did not install tools or change global ASLR. P4 remains partial; the ten-minute course soak, literal committed clean-clone verification, optimisation evidence and final submission materials remain pending.
## Repository and AI declaration

`src/common` holds simulation, protocol, snapshots, security and audit; `src/server` owns authoritative state; `src/client` owns rendering/input and snapshot history. `idl` holds bounded wire types, `tests` contains behavior/negative harnesses, and `tools` contains setup/provisioning/launch/check helpers.

AI assistance was used and must be declared in the zip, slides and video. `docs/AI_USAGE.md` records generated work, independent review and actual testing. Team: Stanley Lur (leader), Jordain Ng; no team name. Actual contributions, human narrated video and final slide authorship remain for the team to fill in; no human contribution is invented.


## Current local integration

See docs/INTEGRATION.md before relying on historical test claims. Current controls are `bash tools/play.sh --security-dir <PRIVATE_LINUX_BUNDLE>` and `bash tools/stop.sh`; explicit `--baseline` retains the P0 reference. Game sessions retain unique logs. Existing explicit audit paths are never overwritten; choose a fresh path, or omit --audit-file for a unique default. The original contributor play_local.sh remains available.

Team: Stanley Lur (leader), Jordain Ng; no team name. Module responsibilities are supplied by the team, not inferred from AI-generated code.


## Current finish-mode limitations and submission preparation

- Leave ACK reliability is unresolved. With explicit user approval, only a transport ACK timeout after a successful Leave write warns and permits normal exit. Other errors remain fatal. Fresh server slot-release events and replacement joins are independently tested; a client warning does not prove receipt.
- Existing TSan dependency race/mutex-lifetime and Valgrind uninitialized crypto-send findings remain open, unsuppressed and outside this finish scope. Current runs do not establish sanitizer cleanliness.
- Current configuration is same-machine domain42/loopback. Two-computer/LAN/VPN testing, further GUI investigation, slides and video production are outside this finish scope and remain team requirements/limitations.
- External packet confidentiality evidence is pending the user's actual capture and unprivileged analysis. The helper's offline checks are not ciphertext proof. Controlled idle CPU comparisons and hardware perf measurements are absent.
- Fresh-source verification used GitHub base3dae8e3 plus the current uncommitted working patch. The latest changes are not claimed to be published or part of a final committed archive.

[RUN_AND_VERIFY.md](docs/RUN_AND_VERIFY.md) contains exact terminal commands, manual checks, capture instructions and source-ZIP instructions. Review/commit intended source changes with GitHub Desktop first; archive that reviewed commit only after the explicit filename-exclusion check. Keep credential bundles, private keys/PEM, build output and captures outside the ZIP. Inspect and build a fresh extraction before submission; no final ZIP or submission has been claimed.
