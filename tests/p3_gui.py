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
        command("import", "-window", window, path)
        assert command("identify", "-format", "%wx%h", path) == "800x600"
        print(f"SCREENSHOT {path}", flush=True)


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
    if remaining > 0:
        time.sleep(remaining)
    snapshots(out, windows, "10s")
    assert all(process.poll() is None for process in [server, *clients])
    for process in clients:
        assert stop(process) == 0, "GUI client shutdown failed"
    assert stop(server) == 0, "Server shutdown failed"
    summary = re.search(r"SERVER_SUMMARY ([^\n]+)", server_log.read_text())
    assert summary, "No server summary"
    counts = {key: int(value) for key, value in re.findall(r"(\w+)=(\d+)", summary.group(1))}
    for key, minimum in (("joins", 2), ("peak", 2), ("moves", 1), ("shots", 1), ("snapshots", 1)):
        assert counts.get(key, -1) >= minimum, f"GUI did not exercise {key}: {counts}"
    print("SERVER_SUMMARY", summary.group(1), flush=True)
    print("PASS secure two-GUI lifecycle, real key input and >=10s rendering; inspect screenshots", flush=True)


def main():
    """Launch secured server and two distinct visible windows; always clean own children."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--security-dir", required=True)
    parser.add_argument("--bin-dir", default="build/release")
    args = parser.parse_args()
    out = Path("build/p3-gui")
    out.mkdir(parents=True, exist_ok=True)
    binaries = Path(args.bin_dir).resolve()
    environment = dict(os.environ, LIBGL_ALWAYS_SOFTWARE="1")
    processes, streams = [], []

    def start(name, kind, extra):
        path = out / f"{name}.log"
        stream = path.open("w")
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
