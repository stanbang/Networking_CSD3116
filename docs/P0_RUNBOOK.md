# P0 baseline runbook

Purpose: reproducible P0 verification and user controls. CSD3116 team with Codex assistance, 4 November 2026 (Singapore).

P0 is the unchanged lecturer square demo. It is insecure and is not the asteroid game. Do not use these scripts as a production fallback in later phases. Follow `PHASE_APPROVALS.md`; no phase or security decision is automatically approved.

## Prerequisites and current limit

Ubuntu preflight now confirms gcc, make, idlc, CycloneDDS, GLFW, OpenSSL, valgrind, xvfb-run/Xvfb, ImageMagick import, tcpdump and xdotool. The following human install command was needed earlier and has now been satisfied:

```bash
sudo apt install -y xdotool
```

No agent sudo is permitted. Check installation before claiming the GUI test can run.

## Automated P0 GUI verification

From Ubuntu, with no other baseline process running:

```bash
cd /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116
make baseline
xvfb-run -a env LIBGL_ALWAYS_SOFTWARE=1 python3 -B tests/p0_gui_smoke.py --output build/evidence/p0-gui-run1
```

Use a new output directory for each attempt; the harness refuses to overwrite prior evidence. It starts the server and two sequentially joined clients on an isolated X display, saves before/after PNGs and logs, injects arrow keys, checks that each intended square moves and both clients show matching positions, and keeps both clients alive for at least ten seconds. It stops only its own child processes. Xvfb isolates the display, not the baseline DDS domain or network.

The harness must exit 0 and print a measured PASS summary to count as an automated GUI pass. Independently inspect its actual PNGs. A syntax check, synthetic image test, package install or this runbook is not a GUI pass. Baseline graphics have labelled squares, not ships/asteroids/HUD security states.

## Desktop play and stop

After automated checks pass, run in an ordinary Ubuntu/WSLg terminal:

```bash
cd /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116
bash tools/play.sh
```

This starts the baseline server first, then waits for protocol activity from each client before starting the next. Both client windows use the same baseline title. The launcher prints log paths and does not claim visual success from process creation. Confirm actual visible Player 1 and Player 2 windows, click inside each, and use arrow keys. The baseline is CPU-intensive and has no crash-leave support; those defects remain on the later-phase plan.

Stop only this launcher's recorded session:

```bash
cd /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116
bash tools/stop.sh
```

The stop helper validates process identity and uses Linux pidfds. It will not guess or kill unrelated processes. If it reports an ownership mismatch, inspect the recorded state/logs and close the original terminal/window manually. Do not use broad kill-by-name commands. The play/stop helper and test harness share a lock to prevent overlapping managed runs.

## Human packet capture

The human, not the agent, opens another Ubuntu terminal while the baseline is running:

```bash
cd /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116
sudo bash tools/capture_demo.sh 30 p0-demo.pcap lo
```

The final argument selects one interface. Retry `lo` with the corrected filter first; if the capture is still empty, try an observed active interface such as `eth0` with a different output basename. The script rejects interface `any`. Cyclone DDS can choose ephemeral UDP ports for game data, so the old 7400-7600 port range was insufficient. The corrected filter is `ip and udp and udp[8:4] = 0x52545053`: it matches the IPv4 UDP payload's `RTPS` header regardless of port, based on the installed `dds/ddsi/q_protocol.h` header. This selects a protocol signature, not authenticated participants or one DDS domain; other IPv4 RTPS traffic on that interface can be included. It does not cover IPv6 or reassemble IP fragments. The P0 messages are small; later protocol evidence needs its own capture plan. Time remains capped at 120 seconds, packet count at 10000 and per-packet capture at 256 bytes, with truncation explicit.

The script uses the invoking user's Linux home from the password database, validates ownership and symlinks, and verifies capture directory mode 0700/file mode 0600. Output is under `~/.local/state/dds-asteroids/captures/`, not Windows-mounted source storage. It prints the actual path and an unprivileged inspection command. It refuses existing output and publishes only a nonempty parseable capture. Do not commit or package packet captures; `*.pcap` is ignored in the repository as requested.

After capture, analysis can run without sudo:

```bash
tcpdump -n -r ~/.local/state/dds-asteroids/captures/p0-demo.pcap
```

The earlier human capture with the old port filter contained zero packets and provides no game-traffic evidence. A fresh nonempty capture with the corrected filter still needs unprivileged analysis. P0 capture is a plaintext reference, not proof of encryption, authentication or authorization. Security evidence is required after a separately approved mechanism is implemented.

## Approval gate

Once P0 automated checks and desktop launch are actually verified, the orchestrator will print a numbered manual checklist and STOP. The next phase cannot start until the user replies `APPROVED P0` or `FIX: ...`. Security selection and breaking IDL changes need their own explicit approval. xdotool is installed and run2 GUI evidence passed. The user approved P0. Human capture retry is pending; security selection and breaking IDL approval remain separate gates.


P0 approval received: user said "Approved. instructions work". Retry capture with the corrected RTPS filter while the restored demo runs: `sudo bash tools/capture_demo.sh 30 p0-retry.pcap lo`. If still empty, retry with `eth0` and a different basename. No real capture has been analysed yet.
