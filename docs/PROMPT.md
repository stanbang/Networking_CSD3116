# Master Prompt: Secure Multiplayer 2D Asteroid Shooter over DDS

Paste this whole file as the first message to your AI coding assistant, with the repo open
(`baseline/`, `idl/`, `Makefile`, `tools/` already exist). Replace anything in `<angle brackets>`.

---

## 0. How you must work

1. Work in the phases in section 13. After each phase: build, run, test, and report what actually happened. Never claim something works unless you ran it. Paste real compiler and test output.
2. If a requirement is impossible or ambiguous in my environment, stop and ask me. Do not silently substitute.
3. I must be able to explain and defend every line (the brief requires a code walkthrough). Keep the code simple, small functions, no clever tricks. After each phase give me a 5-line "defence notes" summary: what it does, why this design, what could go wrong.
4. Every file gets a header comment (purpose, author `<team>`, date). Comment the why, not the what. Log AI use in `docs/AI_USAGE.md` as you go (prompt summary, what you generated, how it was tested).
5. Priority order when time is short: **security > networking correctness > threading/memory safety > measured optimisation > game polish.**

## 1. Context

- Course: CSD3116 Low-Level Programming (DigiPen SIT, Trimester 1 AY2026/27), Week 1 Team Lab: "Real-Time, Interactive, Distributed and Secure Simulation".
- Team of 2-3 with a leader. Submission on Moodle by **Friday 6 November 2026**: source `.zip` (clearly commented), slides `.pdf`, video `.mp4` (max 5 min, 200 MB, narrated by all members, no AI audio).
- Marking for the program (40% of the assignment): runs = 20%, look and feel = 10%, **optimisation and security = 40%**, structure/modularity = 10%, readability = 10%, comments = 10%. Slides 20%, video 20%, peer evaluation 20%.
- The lecturer gave `baseline/server.c`, `baseline/client.c`, `idl/messages.idl`: a server-authoritative square-moving demo (max 5 clients, join handshake, input to server, positions to clients, collisions). We must build a better game from it.

## 2. Non-negotiable constraints

- Language C (gnu11). WSL2/Ubuntu (macOS acceptable). **OpenGL + GLFW + Cyclone DDS** are mandatory. Text via `stb_easy_font.h`. OpenSSL allowed for crypto.
- 2D animation-based, **minimum two players**, real-time, interactive, distributed, secure.
- Must improve on the baseline: support clients leaving; apply class concepts: optimisation, threading, networking, security (authenticate clients, encrypt DDS traffic).
- Start the server first, then clients one by one; each client in its own window (800x600 unless I say otherwise).
- Keep the baseline naming style (`b_`, `int_`, `ch_`, `arr_`, `str_` prefixes) so the code looks consistent to the marker.

## 3. Game design: "DDS Asteroids" (co-op by default)

- Wrap-around 800x600 arena. Each player is a triangular ship: rotate left/right, thrust, fire (space). Ships have inertia and friction.
- Server spawns asteroids (large, medium, small; large splits into medium, and so on). Bullets destroy asteroids, asteroids damage ships. Lives, score, respawn with invulnerability window, wave counter, game-over and restart.
- Optional (decide with me): ship-ship collision as in the baseline, or friendly fire off.
- Fixed-timestep server simulation (60 Hz). Clients send **input state** (key bitmask), not positions. Clients render only what the server sends, with interpolation between snapshots.
- Visuals: line-drawn ships, asteroids, bullets, particle burst on destruction, HUD (player id, lives, score, ping, connection state: "Waiting for server", "Authenticating", "Connected", "Rejected").
- Hard caps (constants in one header): max players 5, max asteroids 64, max bullets 128. All arrays fixed-size.

## 4. Architecture and protocol

Server-authoritative; clients are untrusted. Server main thread = simulation tick; DDS worker thread(s) = network I/O; client main thread = render + input; client DDS thread = network I/O (baseline model, improved).

Redesign `idl/messages.idl` (v2). At minimum:

- `JoinRequest { client_nonce[16 bytes], protocol_version, auth_blob }` and `JoinResponse { client_nonce (echo), status, assigned_player_id, session_info }` so the right client gets the right answer.
- `Input { player_id, seq, tick_ack, key_bitmask }` (+ authentication tag if using the application-layer design).
- `Snapshot { server_tick, ships[<=5], bullets[<=128], asteroids[<=64], scores }` using **bounded** IDL sequences. Compact integer fields; budget the bytes per snapshot and state the bandwidth per client.
- `Leave { player_id }` and liveliness/heartbeat so crashed clients are removed.
- Every message carries a version and sequence number.

Use keyed topics or explicit addressing where a message is for one client only.

## 5. SECURITY (highest priority; this is where most marks and risk are)

### 5.1 Map the lecture's tenets to concrete features

| Tenet (Week 9) | Required in this project |
|---|---|
| Confidentiality | Game traffic unreadable on the wire (prove with Wireshark/tcpdump: ciphertext only) |
| Integrity | Tampered or corrupted messages detected and dropped (AEAD tag or signature) |
| Availability | Join-flood and input-flood resistance; slots freed on leave/timeout |
| Authentication | Every client proves identity with an X.509 certificate signed by our private CA; server also authenticates to clients (mutual) |
| Authorization | A client may only send Join/Input/Leave and read Snapshot/JoinResponse; only the server may publish Snapshot/JoinResponse. A client cannot act as another player |
| Accountability | Server audit log: joins, rejects, auth failures, rate-limit hits, with identity and timestamp. Make it tamper-evident with a SHA-256 hash chain |
| Non-repudiation | At least discuss; implement if time allows (signed join request kept in the audit log) |

### 5.2 Threat model (deliver as `docs/THREAT_MODEL.md` and one slide)

| Threat | Layer | Example here | Required defence |
|---|---|---|---|
| Eavesdropping | Network/Transport | Sniffing UDP to read positions or tokens | Encryption (5.3) |
| Impersonation / ID spoofing | Application | Baseline trusts `int_player_id` inside `Input`; any peer can move any player | Server derives identity from the authenticated session, ignores the id field's authority |
| Tampering / MITM | Network | Alter an Input in flight | AEAD tag / signature, drop on failure |
| Replay | Application | Resend captured Inputs | Sequence numbers + sliding window, session-bound nonces |
| Join flood (SYN-flood analogue) | Transport | Fill all 5 slots with fake joins | Authenticate **before** allocating a slot; per-identity limit; slot lease with timeout |
| Input flood / speed hack | Application | Send 1000 inputs per tick | Server applies at most one input state per tick per player; rate limit; drop excess and log |
| Fake server | Application | Rogue peer publishes Snapshots | Clients accept Snapshots only from the authenticated server identity |
| Malformed messages | Presentation | Out-of-range ids, huge sequences | Validate every field before use (5.5) |
| Memory-safety exploits | Host | OOB index, integer overflow in size calc, UAF, double free, format string | 5.5 and 5.6 |

### 5.3 Mechanism: decision gate (do this first, report to me, then proceed)

Observation from the lectures: TLS (Week 10, OpenSSL `s_server`/`s_client`) runs over **TCP**, while DDS runs over **UDP**, so TLS cannot simply wrap DDS traffic. Therefore:

**Step 1.** Check whether the installed Cyclone DDS has the Security plugins (`bash tools/setup_env.sh` prints this; look for `libdds_security_*`).

**Option A (preferred): native DDS Security.** Authentication (PKI-DH using our identity CA), Access Control (signed permissions per participant, enforcing the authorization row above), Cryptographic plugin (AES-GCM, encrypt + authenticate payloads). Configure through a `CYCLONEDDS_URI` XML file in `config/`. Governance and permissions XML must be signed (S/MIME) with the permissions CA using OpenSSL; extend `tools/gen_certs.sh` to do this and to generate per-client permissions. If apt's build lacks the plugins, build Cyclone from source with security enabled (README section 2). Verify option names against the current Cyclone DDS documentation; do not guess XML element names.

**Option B (fallback or complement): application-layer security with OpenSSL.**
- Mutual authentication: server sends a random nonce; client signs (nonce, client_nonce, transcript hash) with its RSA private key (SHA-256); server verifies against the client certificate chained to our CA; server proves itself the same way.
- Key establishment: ECDH (preferred) or RSA-encrypted session key; derive per-direction keys with a KDF.
- Per-message AEAD: AES-256-GCM with a unique 96-bit nonce (direction bit + counter), AAD = header (player id, seq, tick). Reject on tag failure or old sequence.
- Use OpenSSL EVP APIs only. **Never** invent crypto, never use ECB, never hard-code keys, use `RAND_bytes`, compare secrets with `CRYPTO_memcmp`, wipe keys with `OPENSSL_cleanse`.

**Option C (optional extra):** TCP + TLS 1.3 control channel (OpenSSL server/client as in the lecture) for authentication and key distribution, then DDS data protected as in B.

Default: A; fall back to B if A is unavailable. Tell me what you found and what you chose, and why.

### 5.4 Key and certificate management

- Use the Week 10 workflow: RSA >= 2048 bits, SHA-256, private CA, CA-signed X.509 identity certificates (`tools/gen_certs.sh`).
- **Private keys are never committed, zipped or shown on slides.** Files mode 600. `.gitignore` already blocks them; add a pre-commit check that fails on `*.key`/`*.pem`.
- Clients must trust only our CA (the lecture's client rejected an untrusted self-signed cert; demonstrate the same negative result).
- Document expiry/renewal and what we would do on key compromise (short-lived certs, revocation limits).

### 5.5 Secure coding rules (from Weeks 8 and 9)

- Validate **every** value from the network before use: player id in [1, MAX], array index in range, sequence lengths <= bounds, enums known, ticks plausible. The baseline indexes `arr_Players[id-1]` unchecked (id 0 gives index -1).
- Integer-safe size arithmetic (check before multiplying/adding; no narrowing; beware `uint8_t` style overflow as in the Week 9 heap lecture).
- No `printf(user_data)`; always a constant format string. Never log or display pointers (address leaks defeat ASLR).
- Bounded string/memory functions only. Initialise all structs (`= {0}`), including messages sent over DDS.
- Avoid dynamic allocation in the game loop. Use fixed pools or arenas to prevent leaks, use-after-free and double free. Set pointers to NULL after free.
- Check every return code (DDS, OpenSSL, pthread). Do not continue after `dds_create_participant` fails (the baseline does).

### 5.6 Compile-time and run-time protection (prove it)

- Release flags (already in `Makefile`): `-O2 -Wall -Wextra -Wformat-security -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE -pie -Wl,-z,relro,-z,now`. Zero warnings.
- `make check-hardening` must show: PIE (type DYN), NX (GNU_STACK without E), RELRO, stack canary references. Confirm ASLR = 2.
- Separate sanitizer builds: ASan+UBSan (`make SAN=asan`), TSan (`make SAN=tsan`; cannot be combined with ASan), plus Valgrind (leaks, uninitialised values). All must run clean through a 5-client session with join/leave/rejoin.
- Be able to explain what canary, NX, ASLR and PIE each stop and do **not** stop (Week 8 summary table).

### 5.7 Security evidence (negative tests; each becomes a slide row and a video clip)

| Test | Expected result |
|---|---|
| Sniff traffic with Wireshark/tcpdump | No readable positions, ids or tokens |
| Client with certificate from a different CA | Rejected, logged, no slot used |
| Client with no certificate | Rejected, logged |
| Valid client sends Input claiming another player's id | Ignored/rejected, logged |
| Replay a captured Input | Dropped (sequence window) |
| Flip a byte in a message | Dropped (tag/signature failure) |
| 1000 join attempts per second from one identity | Rate-limited; legitimate players unaffected; slots not exhausted |
| Input flood from a valid client | Movement speed unchanged; excess dropped and counted |
| Out-of-range ids / oversized sequence (mutation test) | No crash, no ASan/UBSan report |
| Client killed with SIGKILL | Server frees the slot within the lease time |

## 6. NETWORKING

- QoS explicitly set (`dds_create_qos`, `dds_qset_*`), not defaults. Suggested: Join/JoinResponse/Leave reliable + keep-all (small); Input best-effort/keep-last(1) with redundancy via sequence numbers; Snapshot best-effort/keep-last(1); liveliness with a 2-3 s lease to detect departure. Justify each choice (DDS trades re-transmission for throughput; reliability is provided at the DDS layer).
- **Replace polling with waitsets or listeners** (`dds_create_waitset`/`dds_waitset_wait`, or data-available listeners). The baseline loops spin at 100% CPU.
- Server tick 60 Hz, snapshot rate 20-30 Hz, client interpolation buffer ~100 ms. Report measured latency and bandwidth.
- Fix the baseline join bug: all waiting clients receive every `JoinResponse` on a shared topic and would adopt the same id. Use the echoed `client_nonce`.
- Configuration: `config/cyclonedds.xml` with domain id, network interface and unicast peers so it works on WSL2 NAT, a LAN, and loopback. Document the UDP port range used and firewall needs.
- Test matrix: loopback; two WSL instances or two machines; simulated loss/latency (`tc netem` if the kernel supports it, otherwise an in-app `--sim-loss`/`--sim-delay` flag).
- Snapshot size must be bounded and documented; note DDS fragmentation if it exceeds one datagram.

## 7. OPTIMISATION (must be measured, not asserted)

Apply concepts from the lectures, with before/after evidence for each:

- **Compiler and registers (Weeks 1-3):** compare `-O0` vs `-O2` with `perf stat`; inspect assembly (`-S -masm=att`) of one hot function.
- **Branching (Week 3, 5):** make a hot loop branchless where branches are unpredictable (clamps, wrap-around); show `branch-misses` change. Remember `-O2` can optimise loops away, so print or use the result; `-fno-if-conversion` for branch studies.
- **Cache (Week 3, 6):** struct-of-arrays for bullets/asteroids, contiguous arrays, row-major access, 64-byte alignment; show `L1-dcache-load-misses` change.
- **Superscalar / dependency chains (Week 5):** multiple accumulators or unrolled loops in per-frame sums (for example collision candidate scans); report IPC rise, **interpreted together with instruction count and time** as the lecture warns.
- **Data-level parallelism (Week 4):** SSE2 (`-msse2`) batch update of positions/velocities.
- **Algorithmic:** spatial grid broad-phase instead of O(n^2) collision checks, with a benchmark at the cap sizes.
- **Memory (Week 9):** fixed pools, no tiny mallocs (a 1-byte malloc costs 32 bytes), no per-frame allocation.
- **Rendering:** cache vertex data for static text; do not rebuild `stb_easy_font` buffers or `snprintf` every frame for unchanged strings.
- **Idle CPU:** waitsets/condition variables instead of spin loops; show CPU% before/after for server and client.

Method (from the Week 3 profiling lecture):
- Build a **headless benchmark** in `bench/` (no DDS, no GL) running hundreds of ms to seconds so `perf` can read counters.
- `perf stat -r 20 -e cycles,instructions,branches,branch-misses,L1-dcache-loads,L1-dcache-load-misses ./bench`. Always compare against a baseline version, compiled with `-O2`.
- **perf does not work in WSL.** Run benchmarks on the course Linux server over SSH. It is shared: short runs only, no long-lived processes, or access may be removed. If unavailable, fall back to `valgrind --tool=cachegrind` and wall-clock timing (`clock_gettime`), and say so clearly.
- Write results to `docs/PERFORMANCE.md` as tables: baseline vs optimised for time, instructions, IPC, branch-miss rate, L1 miss rate, CPU%.

## 8. THREADING

- Keep the main + DDS-thread split. Fix the baseline: never block (`dds_sleepfor`, `dds_write`) while holding a mutex; lock/unlock strictly paired (no early `break`/`return` while locked); copy shared state out under the lock, then work on the copy.
- No unlocked reads of shared variables (the baseline reads `int_num_active_players` and the client's `int_player_id` unprotected). Use mutexes or `_Atomic`.
- Replace spin loops with condition variables or waitsets. Consider a double-buffered snapshot handed from network thread to render thread.
- Justify the thread count (Amdahl's Law: five players do not need many threads). Run ThreadSanitizer clean.

## 9. Known defects in the baseline (fix and mention in the slides)

| # | Defect | Where | Fix |
|---|---|---|---|
| 1 | Unchecked `arr_Players[id-1]` from the network (out-of-bounds, id 0 -> index -1) | server input handler, client position handler | Range-check; ignore invalid |
| 2 | Player id taken from message content, no authentication | `Input` | Session-bound identity |
| 3 | `JoinResponse` has no addressing; simultaneous joiners get the same id | join flow | Echo `client_nonce` |
| 4 | Busy-wait loops (server main + worker, client worker) | all threads | Waitsets / condvars |
| 5 | Mutex held across 20 ms sleeps and DDS writes; sequential position sends | server worker | Short critical sections, batch snapshot |
| 6 | Shared variables read without lock | `int_num_active_players`, client `int_player_id` | Lock or atomic |
| 7 | Participant creation failure ignored | both `worker()` | Check and exit |
| 8 | No leave/timeout; slots never freed | both | Leave + liveliness lease |
| 9 | Unbounded input rate (movement speed set by client send rate) | `Input` path | Per-tick rate cap |
| 10 | No sequence/timestamp on messages; no replay defence | all topics | seq + window |
| 11 | Samples allocated but never freed; entities never deleted | both | Clean shutdown |
| 12 | Placement loop `while(1)` runs under the mutex | server join | Bounded retries |

## 10. Code structure and quality (30% of program marks)

- Modules: `protocol.h` (constants, validation), `net_server.c`, `net_client.c`, `sim.c` (pure game logic, no I/O, unit-testable), `render.c`, `security.c`, `audit_log.c`, `config.h` (every constant; no magic numbers).
- Functions <= ~50 lines, one job each, consistent naming per the baseline prefixes, header comments on every function (parameters, return, threading/locking assumptions).
- `-Wall -Wextra` clean. `make`, `make SAN=asan`, `make SAN=tsan`, `make check-hardening` must all work from a clean clone.

## 11. Testing

- `make test`: unit tests for `sim.c` (collisions, wrap, splitting) and for each validator (feed boundary and malicious values).
- Soak test: 5 clients for 10 minutes with random join/leave/rejoin, sanitizers on.
- Security negative tests from 5.7, scripted where possible (`tests/`).

## 12. Material for the other deliverables

- **Slides (PDF):** build/run instructions; who did what; game concept, mechanics, design (component diagram, join/auth sequence diagram, client state machine, data/structure diagram); optimisation with profiler tables; security features with the threat model and negative test results; AI-use declaration and proof of testing (walkthrough and defence of accepted output). Large fonts, little text per slide (clarity 25%, structure 25%, technical content 50%).
- **Video script (<= 5 min):** team and member identification, game demo, optimisation highlights, security demo (sniffing, rejected client). All members narrate, no AI voice, can be AI-captioned.
- **AI declaration** must appear in the zip (`docs/AI_USAGE.md`), the slides and the video.

## 13. Phases (cut line: finish P0-P3 before touching polish)

| Phase | Goal | Done when |
|---|---|---|
| P0 | Environment + baseline running with 2 clients; report Cyclone security-plugin result and WSL display/multicast status | baseline built, both windows move squares |
| P1 | Protocol v2 + fixes for defects 1-3, 7, 8, 12; join/leave/timeout | clients can leave and rejoin; no OOB writes |
| P2 | Security (A or B), threat model, negative tests | 5.7 table passes with evidence |
| P3 | Asteroid game sim + rendering on the new protocol | playable by 2+ players |
| P4 | Threading and memory-safety fixes; sanitizers clean | ASan/UBSan/TSan/Valgrind clean |
| P5 | Optimisation with profiler evidence | `docs/PERFORMANCE.md` filled |
| P6 | Slides outline, video script, AI log, final zip | package matches the Moodle checklist |

## 14. Open decisions (ask me before assuming)

1. Co-op or competitive? (default: co-op)
2. Security Option A, B, or both? (default: A, fallback B)
3. Do we have access to the course Linux server for `perf`?
4. Is WSLg working on every teammate's machine, and are we testing across machines?
5. Team size and who owns which module (needed for the "who did what" slide).
