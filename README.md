> **Paused teammate checkpoint:** read [docs/HANDOVER.md](docs/HANDOVER.md). Active GUI lease loss is repaired in the latest run; overall acceptance still fails a native Leave ACK timeout. This is not a completed release.

> Current local integration: fresh build/unit/hardening/headless lifecycle checks pass, but the secure two-window GUI check FAILED with lease loss and stale game state. See docs/INTEGRATION.md. Contributor historical passes below are not fresh verification on this host. Do not treat this as submission-ready.

# DDS Asteroids

Secure, server-authoritative co-op Asteroids in C, using Cyclone DDS, OpenGL, GLFW and OpenSSL. CSD3116, DigiPen SIT. Author: <team>, with declared Codex assistance; updated 2026-11-04.

**Playable locally:** five-player networking, native DDS Security and gameplay/GUI tests pass. Hardening and a two-minute ASan/UBSan five-client churn pass. Full memory-safety verification remains incomplete: TSan reports in the uninstrumented system DDS library and Valgrind uninitialised-byte reports in its crypto send path require investigation. See `docs/TEST_REPORT.md` and `docs/PROGRESS.md`. LAN play, human feel and external Wireshark inspection require human checks.

## Quick start on this Windows machine

Run this in Windows Command Prompt or PowerShell:

```cmd
wsl -d Ubuntu --cd /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116 -- bash tools/play_local.sh
```

Or, inside Ubuntu:

```bash
cd /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116
bash tools/play_local.sh
```

The launcher builds the game, prepares private Linux credentials and opens two separate player windows. Click a window to control that player. Closing either window ends this local session. Logs are in `build/local-*.log`. Default session limit is one hour; use `--duration 120` for two minutes. `--fresh-certs` regenerates the private fixture, and `--headless --duration 5` runs two terminal bots. The launcher itself and the actual two-window game have been tested separately; GUI evidence uses Xvfb/software OpenGL.

| Control | Action |
|---|---|
| Up | Thrust |
| Left / Right | Rotate |
| Space | Fire |
| R | Restart after shared game-over |
| Escape | Leave / close window |

Ships have inertia/friction and wrap across all edges. Large rocks split into medium, then small; kills award 20/50/100 points. Three lives per player; a hit causes a one-second respawn and two seconds of protection. Friendly fire is off. The HUD shows all players' lives/scores, wave, connection state and an input-echo RTT estimate. See `docs/P3_GAME.md` for mechanics, snapshot bounds and threading.

## Requirements and build

Ubuntu 24.04/WSL2, GCC/C11, make, pkg-config, Cyclone DDS development/tools, GLFW3, OpenGL, OpenSSL development headers. Graphical play needs WSLg or an X11 display. This machine has Cyclone DDS 0.10.4, GLFW 3.3.10 and OpenSSL 3.0.13; other package layouts have not been verified.

A human can install dependencies inside Ubuntu:

```bash
sudo apt update
sudo apt install -y build-essential pkg-config git gdb valgrind openssl libssl-dev libgl1-mesa-dev libglfw3-dev cyclonedds-dev cyclonedds-tools xvfb xauth imagemagick xdotool tcpdump
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

```bash
./build/release/server --security-dir /tmp/YOUR_PRIVATE_BUNDLE --duration 3600
# Separate terminals, distinct identities:
./build/release/client --security-dir /tmp/YOUR_PRIVATE_BUNDLE --identity 1 --duration 3600
./build/release/client --security-dir /tmp/YOUR_PRIVATE_BUNDLE --identity 2 --duration 3600
```

Headless clients accept `--headless --bot idle|circle|straight|hunter`. Server also accepts `--ticks N`. `--test-scenario gameover` is an explicit server-only collision fixture for automated testing; it changes setup, is off by default, and is not normal gameplay.

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
python3 tests/p1_network.py --security-dir /tmp/YOUR_PRIVATE_BUNDLE
python3 tests/p3_gameplay.py --security-dir /tmp/YOUR_PRIVATE_BUNDLE
python3 tests/p3_gui.py --security-dir /tmp/YOUR_PRIVATE_BUNDLE
cc -shared -fPIC -Wall -Wextra tests/rtps_fault.c -o build/rtps_fault.so -ldl -pthread -lcrypto
python3 tests/p2_security.py --security-dir /tmp/YOUR_PRIVATE_BUNDLE
```

The security harness uses genuine post-encryption ciphertext corruption, actual native GCM failure and separately chained production diagnostics; it does not substitute a plaintext change before encryption. No-cert discovery is logged as an unauthenticated observation with identity 0 under denying governance, not as an invented certificate failure. The forbidden server-publication test is denied locally by native access control; it does not invent a remote server event for a packet never sent.

Actual unit, five-bot, lifecycle, security, controlled game-over/restart and two-window results are in `docs/TEST_REPORT.md`; findings are in `docs/REVIEW_LOG.md`. Human checks of smoothness, controls, LAN play and Wireshark are in `docs/PLAYTEST.md`. Later phases cover sanitizers, Valgrind, measured optimisation and submission materials; their status is explicit in `docs/PROGRESS.md`.

## Remaining verification prerequisite

The playable release, unit/physics/security checks, five-client gameplay, controlled restart, two GUI windows, fail-closed hardening and ASan/UBSan churn have actual passing evidence. Full TSan and Valgrind acceptance **does not pass**. Disabling ASLR only for the TSan test processes fixes startup mapping, but the actual game still reports DDS-library races. Valgrind shows zero heap left at exit in all six processes but reports uninitialised bytes in client native-crypto sendmsg paths. No suppressions or blanket false-positive claims were used.

A matching instrumented dependency build/investigation needs CMake, which is missing inside Ubuntu. The human can run there:

```bash
sudo apt update
sudo apt install -y cmake
```

This is a prerequisite for investigation, not a promised fix. Agents did not install tools or change global ASLR. P4 remains partial; the ten-minute course soak, literal committed clean-clone verification, optimisation evidence and final submission materials remain pending.
## Repository and AI declaration

`src/common` holds simulation, protocol, snapshots, security and audit; `src/server` owns authoritative state; `src/client` owns rendering/input and snapshot history. `idl` holds bounded wire types, `tests` contains behavior/negative harnesses, and `tools` contains setup/provisioning/launch/check helpers.

AI assistance was used and must be declared in the zip, slides and video. `docs/AI_USAGE.md` records generated work, independent review and actual testing. Team names, contributions, human narrated video and final slide authorship remain for the team to fill in; no human contribution is invented.


## Current local integration

See docs/INTEGRATION.md before relying on historical test claims. Current controls are `bash tools/play.sh --security-dir <PRIVATE_LINUX_BUNDLE>` and `bash tools/stop.sh`; explicit `--baseline` retains the P0 reference. Game sessions retain unique logs. Existing explicit audit paths are never overwritten; choose a fresh path, or omit --audit-file for a unique default. The original contributor play_local.sh remains available.

Team: Stanley Lur (leader), Jordain Ng; no team name. Module responsibilities are supplied by the team, not inferred from AI-generated code.
