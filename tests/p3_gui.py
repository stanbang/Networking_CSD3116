#!/usr/bin/env python3
"""Secure two-window game smoke test under Xvfb. Author: <team>/Codex; 2026-11-04.

Run: xvfb-run -a -s '-screen 0 1620x650x24' python3 tests/p3_gui.py --security-dir DIR
Screenshots require visual inspection in addition to automated lifecycle checks.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import time
sys.dont_write_bytecode = True
from p1_network import stop, wait_line


def command(*args):
    """Return bounded external tool output; fail on nonzero status."""
    return subprocess.check_output(list(map(str, args)), text=True, timeout=8).strip()


def window_for(process):
    """Require both PID and title: xdotool's default OR can select the wrong peer."""
    deadline = time.monotonic() + 8
    while time.monotonic() < deadline:
        result = subprocess.run(["xdotool", "search", "--all", "--onlyvisible", "--pid",
                                 str(process.pid), "--name", "DDS Asteroids"],
                                capture_output=True, text=True, timeout=2)
        if result.returncode == 0 and result.stdout.strip():
            return result.stdout.splitlines()[0]
        time.sleep(.1)
    raise AssertionError(f"No GUI window for child {process.pid}")


def snapshots(out, windows, label):
    """Retain each distinct client window at native resolution for visual review."""
    for identity, window in enumerate(windows, 1):
        path = out / f"client{identity}-{label}.png"
        command("xdotool", "windowraise", window)
        time.sleep(.2)
        command("import", "-window", window, path)
        assert command("identify", "-format", "%wx%h", path) == "800x600"
        print(f"SCREENSHOT {path}", flush=True)


def assert_live_state(out):
    """Require distinct admission and continuing own-state receipt before shutdown."""
    identities, last_ticks = [], []
    for number in (1, 2):
        text = (out / f"client{number}.log").read_text(errors="replace")
        joins = re.findall(r"^CLIENT_JOIN .* id=([1-5]) status=accepted$", text, re.M)
        assert len(joins) == 1, f"Client{number} must have one accepted assignment: {joins}"
        identity = int(joins[0])
        snapshots = [(int(tick), int(player)) for tick, player in
                     re.findall(r"^CLIENT_SNAPSHOT tick=(\d+) id=(\d+) ", text, re.M)]
        assert len(snapshots) >= 9, f"Client{number} own-state stream stopped: {snapshots}"
        assert all(player == identity for _, player in snapshots), "Snapshot ownership changed"
        ticks = [tick for tick, _ in snapshots]
        assert ticks[-1] - ticks[0] >= 9 * 60, f"Client{number} insufficient ongoing state: {ticks}"
        assert all(0 < later - earlier <= 2 * 60 for earlier, later in zip(ticks, ticks[1:])), \
            f"Client{number} stale or gapped state stream: {ticks}"
        identities.append(identity)
        last_ticks.append(ticks[-1])
        print(f"LIVE_STATE client={number} id={identity} samples={len(ticks)} "
              f"first_tick={ticks[0]} last_tick={ticks[-1]}", flush=True)
    assert len(set(identities)) == 2, f"Clients share an assignment: {identities}"
    assert abs(last_ticks[0] - last_ticks[1]) <= 2 * 60, "Client state streams diverged"


def watch_snapshot_progress(out, seconds):
    """Observe new own-snapshot log ticks; existing old lines are not fresh evidence."""
    previous, progressed = {}, {}
    for number in (1, 2):
        text = (out / f"client{number}.log").read_text(errors="replace")
        ticks = re.findall(r"^CLIENT_SNAPSHOT tick=(\d+) ", text, re.M)
        previous[number] = ticks[-1] if ticks else None
        progressed[number] = None
    deadline = time.monotonic() + max(seconds, 2.1)
    while time.monotonic() < deadline:
        for number in (1, 2):
            text = (out / f"client{number}.log").read_text(errors="replace")
            ticks = re.findall(r"^CLIENT_SNAPSHOT tick=(\d+) ", text, re.M)
            current = ticks[-1] if ticks else None
            if current is not None and current != previous[number]:
                previous[number] = current
                progressed[number] = time.monotonic()
        time.sleep(.1)
    return progressed


def assert_fresh_progress(progressed):
    """Fail if either stream stopped together or never advanced in the observation window."""
    now = time.monotonic()
    for number in (1, 2):
        observed = progressed[number]
        assert observed is not None, f"Client{number} snapshot stream made no fresh progress"
        age = now - observed
        assert 0 <= age <= 2, f"Client{number} snapshot progress stale by {age:.3f}s"
        print(f"FRESH_STATE client={number} observed_age_seconds={age:.3f}", flush=True)


def exercise(out, server, server_log, clients, windows):
    """Drive real GLFW keys and keep both GUI clients alive for at least ten seconds."""
    begin = time.monotonic()
    time.sleep(1)
    snapshots(out, windows, "before")
    for window in windows:
        command("xdotool", "windowfocus", "--sync", window)
        command("xdotool", "keydown", "Up", "Right", "space")
        time.sleep(1)
        command("xdotool", "keyup", "Up", "Right", "space")
    time.sleep(1)
    snapshots(out, windows, "after-input")
    remaining = 10.5 - (time.monotonic() - begin)
    progressed = watch_snapshot_progress(out, remaining)
    snapshots(out, windows, "10s")
    assert_fresh_progress(progressed)
    assert all(process.poll() is None for process in [server, *clients])
    assert_live_state(out)
    for process in clients:
        assert stop(process) == 0, "GUI client shutdown failed"
    assert stop(server) == 0, "Server shutdown failed"
    summary = re.search(r"SERVER_SUMMARY ([^\n]+)", server_log.read_text())
    assert summary, "No server summary"
    counts = {key: int(value) for key, value in re.findall(r"(\w+)=(\d+)", summary.group(1))}
    for key, minimum in (("joins", 2), ("peak", 2), ("moves", 1), ("shots", 1), ("snapshots", 1)):
        assert counts.get(key, -1) >= minimum, f"GUI did not exercise {key}: {counts}"
    assert counts.get("joins") == 2 and counts.get("peak") == 2, "Expected two simultaneous sessions"
    assert counts.get("timeouts") == 0, f"Unexpected GUI lease loss: {counts}"
    assert counts.get("invalid") == 0, f"Unexpected legitimate input rejection: {counts}"
    assert counts.get("leaves") == 2, f"Both GUI clients must leave normally: {counts}"
    assert not re.search(r"^SERVER_DROP .*reason=session\b", server_log.read_text(), re.M), \
        "Legitimate GUI session was invalidated"
    print("SERVER_SUMMARY", summary.group(1), flush=True)
    print("PASS secure two-GUI lifecycle, real key input and >=10s rendering; inspect screenshots", flush=True)


def main():
    """Launch secured server and two distinct visible windows; always clean own children."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--security-dir", required=True)
    parser.add_argument("--bin-dir", default="build/release")
    parser.add_argument("--output-dir", type=Path, default=Path("build/p3-gui"))
    args = parser.parse_args()
    out = args.output_dir
    out.mkdir(parents=True, exist_ok=False)
    binaries = Path(args.bin_dir).resolve()
    environment = dict(os.environ, LIBGL_ALWAYS_SOFTWARE="1")
    processes, streams = [], []

    def start(name, kind, extra):
        path = out / f"{name}.log"
        stream = path.open("x")
        streams.append(stream)
        child = subprocess.Popen([str(binaries/kind), "--security-dir", args.security_dir,
                                  "--duration", "25", *extra], stdout=stream,
                                 stderr=subprocess.STDOUT, env=environment)
        processes.append(child)
        return child, path

    try:
        server, log = start("server", "server", ["--audit-file", str(out/"audit.log")])
        time.sleep(.5)
        clients, windows = [], []
        for identity in (1, 2):
            process, path = start(f"client{identity}", "client", ["--identity", str(identity)])
            wait_line(path, r"CLIENT_JOIN .*status=accepted", process)
            window = window_for(process)
            command("xdotool", "windowmove", window, (identity-1)*810, 0)
            clients.append(process)
            windows.append(window)
        assert windows[0] != windows[1], "Must exercise distinct GUI windows"
        exercise(out, server, log, clients, windows)
    finally:
        for process in reversed(processes):
            if process.poll() is None:
                try:
                    stop(process)
                except AssertionError as error:
                    print(f"CLEANUP {error}", file=sys.stderr)
        for stream in streams:
            stream.close()


if __name__ == "__main__":
    main()
