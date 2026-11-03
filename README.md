# DDS Asteroids

A real-time, interactive, distributed and **secure** 2D multiplayer asteroid shooter built on
Cyclone DDS, OpenGL and GLFW. Developed for the CSD3116 Low-Level Programming team lab
(DigiPen Institute of Technology Singapore, AY2026/2027, Trimester 1).

> Status: starter skeleton. Items under "Features" are targets; tick them off as they are built and tested.

## Features (targets)

- [ ] 2-5 players, server-authoritative simulation (ships, bullets, asteroids, score, lives)
- [ ] Join, leave and crash-timeout handling (slots are freed)
- [ ] Authenticated clients (X.509 identities signed by a private CA)
- [ ] Encrypted and integrity-protected traffic (no plaintext game data on the wire)
- [ ] Replay protection, input validation and rate limiting
- [ ] Measured optimisations (perf evidence, before vs after)
- [ ] Clean under AddressSanitizer, UBSan, ThreadSanitizer and Valgrind

## Requirements

| Item | Notes |
|---|---|
| OS | Ubuntu 24.04+ or WSL2 (Ubuntu). macOS is allowed by the brief, but the Makefile targets Linux. |
| Compiler | GCC with C11 support |
| Libraries | Cyclone DDS (`cyclonedds-dev`, `cyclonedds-tools`), GLFW 3 (`libglfw3-dev`), OpenGL (`libgl1-mesa-dev`), OpenSSL (`libssl-dev`) |
| Tools | `make`, `pkg-config`, `gdb`, `valgrind`, `openssl`; `perf` only on a real Linux machine |
| WSL GUI | Needs WSLg (Windows 11, or recent Windows 10). Check with `echo $DISPLAY`. |

## 1. Get the code

```bash
git clone <YOUR_REPO_URL> dds-asteroids
cd dds-asteroids
```

## 2. Install the environment

```bash
bash tools/setup_env.sh
```

This installs every dependency and prints a checklist: tool versions, whether DDS Security plugins exist,
display status, and ASLR status. Read the output; it decides which security design you can use.

### If the checklist says DDS Security plugins are missing

The apt package may be built without them. Build Cyclone DDS from source (needs `cmake`):

```bash
sudo apt install -y cmake
git clone https://github.com/eclipse-cyclonedds/cyclonedds.git
cd cyclonedds && mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=$HOME/cdds -DENABLE_SECURITY=ON ..
cmake --build . --parallel && cmake --build . --target install
export CMAKE_PREFIX_PATH=$HOME/cdds
export PKG_CONFIG_PATH=$HOME/cdds/lib/pkgconfig:$PKG_CONFIG_PATH
export LD_LIBRARY_PATH=$HOME/cdds/lib:$LD_LIBRARY_PATH
```

(Check the Cyclone DDS docs for the current option names if the build rejects a flag.)

## 3. Build

```bash
make                 # hardened release build -> build/release/{server,client}
make baseline        # lecturer's original programs, for comparison
make SAN=asan        # AddressSanitizer + UBSan build -> build/asan/
make SAN=tsan        # ThreadSanitizer build -> build/tsan/
make check-hardening # confirms PIE, NX, RELRO and stack canary are present
```

`messages.c` and `messages.h` are generated from `idl/messages.idl` by `idlc` during the build
and are not committed.

## 4. Generate certificates (security)

```bash
bash tools/gen_certs.sh            # writes to security/generated/ (gitignored)
```

Private keys never leave `security/generated/`. Do not copy them into the repo, the zip or the slides.

## 5. Run

Start the server first, then each client in its own terminal (one client per player):

```bash
./build/release/server
./build/release/client      # repeat for each player
```

Controls: arrow keys to turn and thrust, space to fire (click the window first to focus it).

### Playing across machines or WSL instances

DDS discovery uses UDP multicast by default, which WSL2's default NAT networking does not forward.
Either enable mirrored networking in `%UserProfile%\.wslconfig` (`networkingMode=mirrored`), or point
Cyclone DDS at explicit unicast peers via `CYCLONEDDS_URI=file://$PWD/config/cyclonedds.xml`.

## 6. Test and profile

```bash
make SAN=asan && ./build/asan/server        # memory errors, UB
make SAN=tsan && ./build/tsan/server        # data races
valgrind --leak-check=full ./build/release/server
```

`perf` does not work in WSL (no hardware counters). Copy the headless benchmark to the course Linux
server over SSH and run short jobs only (the server is shared):

```bash
gcc -O2 bench/<bench>.c -o bench_x
perf stat -r 20 -e cycles,instructions,branches,branch-misses,L1-dcache-loads,L1-dcache-load-misses ./bench_x
```

Record results in `docs/PERFORMANCE.md` (baseline vs optimised).

## 7. Repository layout

```
idl/           DDS message definitions (messages.idl)
src/common/    protocol, validation, crypto helpers, logging, shared constants
src/server/    authoritative game state, join/leave, networking
src/client/    rendering (GLFW/OpenGL), input, networking
baseline/      lecturer-provided server.c / client.c (read-only reference)
third_party/   stb_easy_font.h
config/        Cyclone DDS XML configuration
security/      security docs; generated keys go to security/generated/ (ignored)
tools/         setup_env.sh, gen_certs.sh
bench/         headless micro-benchmarks for perf
tests/         unit and negative security tests
docs/          PROMPT.md, threat model, performance report, AI usage log
```

## 8. Team and contributions

| Member | Role | Main contributions |
|---|---|---|
| _name_ | Team leader | _fill in_ |
| _name_ | | _fill in_ |
| _name_ | | _fill in_ |

## 9. Declaration of AI use

AI assistance is allowed and must be declared in every deliverable. Maintain `docs/AI_USAGE.md`:
a summary of the prompts used, what was generated, and how each piece was tested and understood.

## 10. Troubleshooting

| Symptom | Likely cause and fix |
|---|---|
| `idlc: command not found` | `cyclonedds-dev` not installed, or install prefix not on `PATH` |
| `cannot open display` / GLFW init fails | WSLg not available; update WSL (`wsl --update`) and restart |
| Clients never join | Multicast blocked; use the unicast peer config (section 5) |
| `perf` reports no events | You are in WSL; use the course Linux server |
| `_FORTIFY_SOURCE redefined` warning | Keep `-U_FORTIFY_SOURCE` before `-D_FORTIFY_SOURCE=2` (already in the Makefile) |
