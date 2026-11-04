# Human playtest checklist

Purpose: observations requiring human judgement. Author: <team>/Codex, 2026-11-04.

This is a checklist to run after automated checks, not a claim that a person has completed it. Record date, machines, participants and any problems below.

## Local two-player session

- Run `bash tools/play_local.sh` from Ubuntu in the repository. Confirm two separate windows open and display different player ids and a connected state.
- Focus each window before using its controls. Check that turning, thrust and firing feel responsive and that the other window shows those actions.
- Confirm ships coast and slow down naturally, wrap at screen edges, and remain distinguishable by colour and HUD identity.
- Shoot asteroids. Check that larger rocks split, small rocks disappear, score rises, and a new wave appears after clearing a wave.
- Fly into asteroids. Check that lives decrease, respawn gives a visible period of protection, and a destroyed ship does not keep firing.
- Lose all remaining lives. Confirm the game-over state and restart action work for the shared game.
- Watch for jitter, long pauses, flashing geometry, unreadable text, surprising input lag and drops in frame rate while firing with several players.
- Resize a window if supported. Check that the arena and HUD remain visible and usable. Repeat on the teammate's display scale.
- Close either window in the local launcher and confirm its other processes exit. With separately launched processes, close or kill one client and confirm the other continues and a new client can join after the lease.

## Two-machine session

- Provision one trusted CA/policy set; give each machine only its own participant bundle from `tools/export_identity.sh`. Never distribute the full provisioning fixture or CA private keys.
- Use a LAN interface and explicit peer configuration rather than the loopback-only default. Record addresses, UDP firewall settings and WSL mirrored/NAT mode actually used.
- Start server first, then clients one at a time. Compare the shared state on both screens.
- Repeat controls, disconnect/rejoin and a longer session. Record perceptible latency, freezes and differences in state.
- Test loss/latency only with a supported, documented method. Record what was actually applied; do not claim the default loopback test covers a LAN.

## Security demonstration

- Run the automated negative tests and inspect genuine rejection events and rate-limit counters.
- Inspect an authorised packet capture in Wireshark. Distinguish encrypted game data from public addressing, timing and certificate/discovery metadata.
- Retain the audit's final root and record count separately, then demonstrate that the verifier rejects a changed or truncated copy.
- Ensure presentation/video/source archives exclude private keys, provisioning fixtures, runtime logs containing unwanted metadata and generated binaries unless explicitly required.

## Results to fill in

Date / testers / machines:

Control and visual observations:

Network configuration and latency observations:

Problems, reproduction steps and decisions:
