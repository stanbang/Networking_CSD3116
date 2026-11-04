# Build, run and verify DDS Asteroids

CSD3116 team with Codex assistance, 5 November 2026. This guide is for Stanley's current Windows/Ubuntu WSL checkout. Commands below are for **Ubuntu**, not PowerShell, unless labelled otherwise. Test one local session at a time. Current DDS configuration is domain42 on loopback, so this is not a two-computer test.

## What changed and what actually passed

The contributor's complete Asteroids implementation was preserved and integrated, rather than rewritten. Earlier local work repaired the observed GUI state/lease regression using bounded reader notifications, made audit files refuse overwrite/symlink targets, strengthened lifecycle/GUI checks and added scoped play/stop tools. Native DDS Security, IDL and gameplay remain in place.

In the slim finish work, the user's explicit Approved T1 allowed exactly one policy change: after a successful Leave write, native ACK timeout emits CLIENT_WARNING and permits normal client exit0. Failed writes, other ACK errors and failed cleanup still fail. This is a known limitation, not a DDS ACK repair. Lifecycle tests now require a fresh server Leave/slot-release event for each graceful client stop and verify replacement admission.

Actual results: incremental client compile exit0; real targeted lifecycle exit0 (13joins,12leaves,1intentional SIGKILL timeout, peak5); explicit public-API timeout injection emitted the warning and exited0 while the real server recorded Leave. Fresh GitHub clone base3dae8e3 plus current uncommitted patch passed build, units and hardening. Two admitted clients ran124.539seconds, joins2/leaves2/timeouts0/invalid0. Strict GUI exited0 and its two new final screenshots were inspected: CONNECTED / DDS SECURE, ships, asteroids and matching HUD state. Full output is in docs/evidence/2026-11-05-slim-finish/. These checks do not prove LAN play, human smoothness, memory-sanitizer cleanliness or external ciphertext capture.

## 1. Prepare and build once

Open Ubuntu from the Start menu. If you only have PowerShell open, type `wsl -d Ubuntu` first. Paste:

```bash
cd /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116
make -j2
make test
make check-hardening
umask 077
mkdir -p "$HOME/.local/state/dds-asteroids"
BUNDLE="$HOME/.local/state/dds-asteroids/credentials-manual-20261105-01"
if [ ! -d "$BUNDLE" ]; then
  bash tools/gen_certs.sh "$BUNDLE"
fi
printf 'Private credential folder: %s\n' "$BUNDLE"
printf 'Display: %s\n' "$DISPLAY"
```

Stop if a build/test command fails. Hardening should report PASS PIE NX RELRO BIND_NOW CANARY for both binaries. DISPLAY must be nonempty for visible windows; an automated Xvfb result does not prove your desktop works. Credentials stay in Linux home, never under /mnt/c or in a ZIP. An existing incomplete/expired manual bundle should not be overwritten; use a fresh suffix such as -02 in all terminals and generate it again.

## 2. Start the server - Ubuntu terminal 1

Use separate terminals so leaving/rejoining a client does not stop the server. Do not also run play.sh during this test.

```bash
cd /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116
BUNDLE="$HOME/.local/state/dds-asteroids/credentials-manual-20261105-01"
AUDIT="build/manual-audit-$(date +%Y%m%d-%H%M%S)-$$.log"
set -o pipefail
./build/release/server --security-dir "$BUNDLE" --duration 600 --audit-file "$AUDIT" 2>&1 | tee "${AUDIT%.log}-server.log"
```

Wait for `SERVER_READY security=NATIVE_DDS domain=42`. Note the printed AUDIT_FILE path. The server has terminal output; the two clients have game windows. The session limit is10minutes, not an instruction to run a long soak.

## 3. Start player 1 - Ubuntu terminal 2

```bash
cd /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116
BUNDLE="$HOME/.local/state/dds-asteroids/credentials-manual-20261105-01"
./build/release/client --security-dir "$BUNDLE" --identity 1 --duration 600
```

Wait for CLIENT_JOIN with status=accepted and the window HUD CONNECTED / DDS SECURE. Note the assigned id; certificate identity and assigned player id are separate concepts.

## 4. Start player 2 - Ubuntu terminal 3

```bash
cd /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116
BUNDLE="$HOME/.local/state/dds-asteroids/credentials-manual-20261105-01"
./build/release/client --security-dir "$BUNDLE" --identity 2 --duration 600
```

Wait for accepted. Both windows should show the same game with distinct assigned player ids.

## 5. Numbered human verification checklist

1. Confirm two800x600 windows, both CONNECTED / DDS SECURE, two distinct players, visible ships/asteroids/HUD. A window stuck WAITING FOR GAME STATE fails this check.
2. Click player1's window. Up thrusts, Left/Right rotate, Space fires. Move and fire for10seconds. Both views should reflect player1's movement; player2 should not respond to keys sent to player1's window.
3. Click player2 and repeat. Check shared asteroid destruction and score changes. Confirm controls feel responsive; automated checks cannot grade feel.
4. Continue for at least30seconds. Neither window should freeze or lose its connected state. Check terminal1 for unexpected SERVER_TIMEOUT during normal active play.
5. Close only player1 with Esc or its window close button. Terminal1 should print SERVER_LEAVE identity=1 with that client's assigned id. Player2 should remain connected. CLIENT_WARNING leave_ack_timeout is the approved limitation, not proof of delivery: the server Leave event is required here.
6. Repeat the exact player1 launch command in terminal2 while the server and player2 remain running. It must be accepted again. Move it and confirm player2 sees it. A rejection/hang fails the rejoin check.
7. If ships are hit, check lives decrease and a respawn occurs. R requests restart when game-over has occurred; pressing it in an active round does not establish game-over/restart behavior. Record anything you could not exercise rather than marking it passed.
8. Close both clients, then press Ctrl+C in the server terminal. Confirm SERVER_SUMMARY and AUDIT_ROOT appear. Both graceful clients should have corresponding Leave events; no unexplained timeout is expected.
9. Verify the audit after the server closes using the block below in terminal1, where AUDIT remains set. It compares the audit with the independently printed root/count in the saved server output. This detects changed/truncated records relative to that saved anchor, but is not a signed non-repudiation proof.

```bash
python3 - "$AUDIT" "${AUDIT%.log}-server.log" <<'PY'
import sys, re
from pathlib import Path
sys.path.insert(0, "tests")
from verify_audit import verify
anchor=re.search(r"^AUDIT_ROOT hash=([0-9a-f]{64}) records=(\d+)$", Path(sys.argv[2]).read_text(), re.M)
if not anchor:
    raise SystemExit("FAIL: no final server audit anchor")
count, root=verify(Path(sys.argv[1]), anchor.group(1), int(anchor.group(2)))
print(f"PASS anchored audit records={count} root={root}")
PY
```

Report PASS/FAIL for each item, the assigned ids, any warning text and the printed audit path. Do not call unexercised game-over, LAN or capture checks passed.

## 6. Automated lifecycle verification, including crash recovery

After stopping the manual server and clients, run:

```bash
cd /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116
BUNDLE="$HOME/.local/state/dds-asteroids/credentials-manual-20261105-01"
python3 -B tests/p1_network.py --security-dir "$BUNDLE" \
  --output-dir "build/manual-lifecycle-$(date +%Y%m%d-%H%M%S)"
printf 'Lifecycle exit: %s\n' "$?"
```

This starts and cleans up its own server/bots, tests five-player capacity, explicit leave/replacement, SIGKILL/lease reclamation, malformed/spoof/replay rejection and audit anchoring. Require exit0 and PASS fresh server slot release, PASS final fault slot permits replacement, and PASS P1 networking lifecycle. A warning alone is insufficient. This is a targeted lifecycle test, not a sanitizer or full security suite.

## Optional managed shortcut for later sessions

Instead of terminals1-3, from one prepared Ubuntu terminal:

```bash
cd /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116
BUNDLE="$HOME/.local/state/dds-asteroids/credentials-manual-20261105-01"
bash tools/play.sh --security-dir "$BUNDLE" --duration 600
```

This starts the server and two windows, printing a unique build/local-session directory containing server.log, client1.log, client2.log and audit.log. It does not prove the windows are visible: confirm them yourself. Closing either client ends this managed session, so use the separate-terminal method above for leave/rejoin tests. To stop the managed session:

```bash
cd /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116
bash tools/stop.sh
```

stop.sh controls only its own managed session, not the separate foreground terminals above. Stop those with Esc and Ctrl+C. Never use broad pkill commands.

## 7. External packet evidence - you run capture

The helper has been changed from a256-byte P0 capture to full65535-byte IPv4 snapshots, bounded4000packets and at most120seconds, retaining private-file/no-overwrite checks. Syntax/argument/offline-filter tests passed; no privileged live capture has yet been verified. It still excludes IPv6 and cannot reassemble IP fragments.

Start this in a fourth Ubuntu terminal before launching the manual game; only you run sudo:

```bash
cd /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116
sudo bash tools/capture_demo.sh 60 secure-manual-20261105-01.pcap lo
```

Then start/play with both clients during that60seconds. The output should report nonzero packets and a saved path under `$HOME/.local/state/dds-asteroids/captures/`. Existing names are refused, so use -02 for another attempt. Zero packets is a failed capture, not encryption evidence. Tell Codex the exact saved path and packet count; Codex will analyze it without sudo. Do not commit or include the pcap in the source ZIP. Capturing packets or failing to find readable strings alone does not prove authenticated encryption.

## Source ZIP instructions after final review

Use GitHub Desktop to review and commit the intended source/docs/test changes, including currently untracked new evidence and test fixtures. Keep all private keys, credential fixtures, captures and build outputs out. Do not archive an old HEAD before committing, because it omits the approved fix and new files.

In Ubuntu, after your commit:

```bash
cd /mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116
git status --short
python3 - <<'PY'
import subprocess
from pathlib import PurePosixPath
paths=subprocess.check_output(['git','ls-tree','-r','--name-only','HEAD'],text=True).splitlines()
forbidden=[p for p in paths if PurePosixPath(p).suffix.lower() in {'.key','.pem','.pcap'} or PurePosixPath(p).parts[0] in {'build','credentials','secrets'}]
if forbidden:
    raise SystemExit('STOP: excluded files in commit: '+', '.join(forbidden))
print('PASS committed filename exclusions; still review source for embedded secrets')
PY
```

Require clean intended source and successful exclusion check. If it fails, stop and fix the commit first. Only then:

```bash
git archive --format=zip --output="../DDS-Asteroids-source-$(date +%Y%m%d-%H%M%S).zip" HEAD
```

Review the ZIP listing and build/test a fresh extraction before submission. Filename exclusions are not a complete secret-content audit. No final archive or submission has been claimed. Slides/video, actual member contributions/narration, peer evaluation, LAN, missing CPU/perf measurements and unsuppressed dependency findings remain disclosed requirements/limitations; see EVIDENCE.md.
