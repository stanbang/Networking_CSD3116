# Asteroids gameplay and snapshot design

Purpose: explain the implemented P3 game and its verification scope. Author: <team>, with Codex assistance. Date: 2026-11-04.

## Playing

Start the secured server first, then clients with distinct identity numbers. Run from the repository root in Ubuntu. Credentials remain in their private Linux directory; see the README and credential export instructions before sharing a participant bundle.

```bash
make -j2 all test
./build/release/server --security-dir /tmp/YOUR_PRIVATE_BUNDLE --duration 120
# Separate terminals:
./build/release/client --security-dir /tmp/YOUR_PRIVATE_BUNDLE --identity 1 --duration 120
./build/release/client --security-dir /tmp/YOUR_PRIVATE_BUNDLE --identity 2 --duration 120
```

Up thrusts; Left/Right rotate; Space fires; R restarts after the shared game-over; Escape closes the window and sends Leave. Ships retain momentum with friction and a 200-pixel/second speed cap. The arena wraps across every edge. Large asteroids split into two medium asteroids, which split into two small asteroids. A destroyed large/medium/small asteroid awards its shooter 20/50/100 points. Friendly fire is off.

Each player starts with three lives. An asteroid hit removes one life, pauses that ship for 60 simulation ticks, then respawns it with 120 invulnerable ticks. A blinking hull indicates invulnerability. Players with no lives remain listed in the HUD. When every connected player's lives are exhausted, the game stops and shows GAME OVER; any connected player may press R to start a new round. Joining an existing game-over does not silently reset everyone: the new player can press R as well. New waves follow after the last asteroid disappears and a 90-tick pause completes.

The HUD shows each player's lives/score, wave, own identity, connection state and an input-echo round-trip estimate. That estimate includes server snapshot scheduling; it is not a measurement of network transit time alone. CONNECTED requires a recent validated snapshot containing the client's own slot. Rejected graphical clients retain their rejection message until window close or their finite duration. A stale stream shows CONNECTION LOST. Restarting a server requires clients to be restarted/rejoined; automatic session recovery is not implemented.

## Authority and threads

`sim.c` is pure deterministic C: no DDS, GLFW, file I/O, threading, or dynamic allocation. The server calls it once per 60-Hz tick. Fixed pools cap ships at 5, bullets at 128 and asteroids at 64. At pool exhaustion a new bullet or split child is omitted; memory and iteration counts stay bounded. Simulation randomness is a deterministic nonsecret PRNG, entirely separate from OpenSSL protocol randomness.

Clients send key state, never positions or damage. The server uses only the latest validated state for each tick; receiving extra packets cannot create extra physics steps or bypass the fire cooldown. Session-generation changes explicitly disconnect/reset the old ship and its bullets, even if a leave/rejoin occurs between two ticks and the occupied-slot bit remains true.

The server main thread owns simulation advancement. Its DDS worker copies a pointer-free frame under the shared mutex, then serializes and publishes it after unlocking. The client DDS worker validates all sequence lengths, ids, positions, health/timers and velocity bounds before copying the loan into an eight-frame history. It returns the middleware loan before rendering can use any data. The GLFW main thread copies two frames under the client mutex and renders without holding it. Position and angle interpolation follow the shortest wrapped displacement with a target delay of 100 ms; rendering never extrapolates arbitrary stale movement.

Particle bursts are cosmetic, inferred from asteroid-id disappearance, and use a fixed 256-particle pool. HUD glyph geometry is cached until its text changes. Their appearance has no effect on authoritative collisions, scores or lives.

## Snapshot bounds and security

The sole authoritative topic is `DDSAsteroids_Snapshot_0`, published only by the server's certificate grant. Existing signed native DDS governance encrypts its RTPS, metadata and serialized payload just as it protects the control topics. No plaintext network fallback was added for the game.

Snapshots have bounded IDL sequences of 5 ships, 128 bullets and 64 asteroids. Positions and velocities are integer hundredths of pixels and pixels/second; angles use an unsigned 16-bit full-circle representation. Ship records also carry score, health, respawn/invulnerability timers, public traveled distance and that client's echoed timestamp. Entity ids distinguish bullet/asteroid pool reuse. Struct padding and pointers are never serialized directly.

A conservative serialized-payload budget is 4 KiB: at most 240 bytes of ship records, 2048 of bullets, 1024 of asteroids, plus bounded headers, counts and alignment. At the target maximum 20 snapshots/second this budgets 80 KiB/second of payload per receiving client, before DDS/security/UDP overhead. These are upper bounds, not measured throughput. The worker's bounded waitsets and scheduling can reduce the realized rate. A full snapshot exceeds a normal Ethernet MTU and can require DDS/IP fragmentation; loopback testing does not establish loss behavior on a real LAN. External packet capture and two-machine play remain distinct checks.

## Automated evidence

`make test` runs malicious protocol boundary tests and independently written simulation behavior fixtures. The simulation fixtures create real collisions and verify wrap, speed/rotation, five-shot/second cooldown, full-pool safety, splitting/scoring, exact respawn delay, invulnerability, game-over and restart. They do not increment evidence counters directly.

`tests/p3_gameplay.py` exercises five secured hunter bots against the real server. `tests/p3_gui.py` launches two secured GLFW clients under Xvfb, injects actual key events, captures both windows and checks that they survive the smoke test. Headless `--bot idle`, `circle`, `straight`, and `hunter` patterns generate ordinary key states; hunter aims using received public snapshots. `CLIENT_SNAPSHOT` logs sampled authoritative coordinates, velocity, score and lives once a second for reproducible assertions. The server summary includes cumulative game counters and simulation ticks. The exact commands/results and visual inspection belong in TEST_REPORT.md.

| Server summary field | Meaning |
|---|---|
| `ticks` | Actual fixed simulation calls, including paused/no-player ticks |
| `moves` | Ship-ticks with nonzero movement; bounded by `ticks * 5`, not a distance |
| `shots` | Successfully spawned pooled bullets |
| `destroyed`, `splits`, `points` | Bullet-destroyed asteroids, parent split events, cumulative points awarded |
| `hits`, `respawns` | Real asteroid damage events and completed respawns |
| `gameovers`, `restarts` | Actual shared game-over and input-driven restart transitions |
| `wraps` | Ship boundary crossings |
| `snapshots` | Snapshot publication attempts; DDS write errors are checked and fail the run |

Counters remain cumulative across a restart, while each player's displayed score resets. Client `distance_mm` is cumulative traveled distance for that connection, avoiding coordinate-wrap ambiguity; velocities in sampled logs are hundredths of pixels/second. The 4-KiB payload budget above includes conservative CDR alignment allowance, but does not claim measured datagram sizes or encrypted traffic rates.

Rare shared game-over/restart transitions have deterministic collision-fixture evidence; a short normal bot run need not naturally reach them. For a separate real-network acceptance case, the server accepts explicit `--test-scenario gameover`. After at least two real sessions join and three simulation seconds elapse, this one-shot local fixture gives those ships one life, clears invulnerability and places stationary small asteroids at their positions. Actual `sim_tick` collisions then remove the lives and enter game-over. Hunter clients see the secured game-over snapshot and send ordinary restart input. The fixture never writes game evidence counters, is off by default, and cannot be activated by a network message. `SERVER_TEST_SCENARIO` labels the artificial setup; `GAME_EVENT` and client state logs report the resulting real transitions. Dedicated runtime evidence belongs in TEST_REPORT.md, separate from normal gameplay.

Human checks of feel, two-machine networking, Wireshark views and long sanitizer/Valgrind runs remain separate acceptance work. No such unperformed check is implied by a successful unit or GUI smoke test.
