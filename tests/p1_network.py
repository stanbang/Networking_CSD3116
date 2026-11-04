#!/usr/bin/env python3
"""P1 black-box networking acceptance. Author: <team>/Codex, 2026-11-03.

Run from repo root after make: python3 tests/p1_network.py.
Writes local command/stdout evidence to build/p1-test. Never claims P2 security.
"""
import argparse
from pathlib import Path
import re
import signal
import subprocess
import sys
import time
sys.dont_write_bytecode = True
from verify_audit import verify


def wait_line(path, pattern, process, timeout=8):
    """Wait with a bound; fail if the process exits before expected evidence."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        match = re.search(pattern, path.read_text(errors="replace"))
        if match:
            return match
        if process.poll() is not None:
            raise AssertionError(f"{path}: process exited {process.returncode}: {path.read_text()}")
        time.sleep(0.05)
    raise AssertionError(f"{path}: missing {pattern!r}: {path.read_text()}")


def stop(process, sig=signal.SIGTERM):
    """Stop only a child owned by this harness; bound cleanup time."""
    if process.poll() is None:
        process.send_signal(sig)
    try:
        return process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        for task in sorted(Path(f"/proc/{process.pid}/task").glob("*")):
            try:
                print("HUNG_THREAD", task.name, (task / "wchan").read_text().strip(), flush=True)
            except OSError:
                pass
        process.kill()
        process.wait(timeout=3)
        raise AssertionError("Child did not stop within five seconds")


def main():
    """Exercise concurrent admission, capacity, leave, lease and bad messages."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bin-dir", default="build/release")
    parser.add_argument("--output-dir", type=Path, default=Path("build/p1-test"),
                        help="New evidence directory; existing output is never overwritten")
    parser.add_argument("--security-dir", help="Use native DDS security when configured by P2")
    args = parser.parse_args()
    security = ["--security-dir", args.security_dir] if args.security_dir else []
    binaries = Path(args.bin_dir).resolve()
    out = args.output_dir
    out.mkdir(parents=True, exist_ok=False)
    audit = (out / "server-audit.log").resolve()
    children, handles = [], []

    def start(name, command):
        path = out / f"{name}.log"
        stream = path.open("x")
        handles.append(stream)
        print("COMMAND", " ".join(map(str, command)), flush=True)
        proc = subprocess.Popen(list(map(str, command)), stdout=stream, stderr=subprocess.STDOUT)
        children.append(proc)
        return proc, path

    def client(name, identity, duration=120, fault=None):
        command = [binaries / "client", *security, "--identity", identity, "--headless",
                   "--bot", "circle", "--duration", duration]
        if fault:
            command += ["--fault", fault]
        return start(name, command)

    accepted = r"CLIENT_JOIN nonce=([0-9a-fA-F]{32}) id=([1-5]) status=accepted"
    try:
        server, server_log = start("server", [binaries / "server", *security, "--duration", 180,
                                               "--audit-file", audit])
        time.sleep(0.5)
        initial = [client(f"initial-{i}", i) for i in range(1, 6)]
        joins = [wait_line(path, accepted, proc) for proc, path in initial]
        assert len({m.group(1) for m in joins}) == 5, "Join nonces must be unique"
        assert len({m.group(2) for m in joins}) == 5, "Concurrent joiners shared a slot"
        print("PASS concurrent joins: five distinct nonces and slots", flush=True)
        sixth, path = client("sixth", 6, 3)
        wait_line(path, r"CLIENT_JOIN .* status=rejected", sixth)
        stop(sixth)
        print("PASS sixth client rejected", flush=True)

        assert stop(initial[0][0]) == 0, "Graceful leaving client failed"
        time.sleep(0.4)
        replacement, path = client("after-leave", 6)
        wait_line(path, accepted, replacement)
        print("PASS explicit leave permits replacement", flush=True)

        stop(initial[1][0], signal.SIGKILL)
        time.sleep(3.5)
        rejoined, path = client("after-crash", 2)
        reclaimed = wait_line(path, accepted, rejoined)
        assert reclaimed.group(2) == joins[1].group(2), "Killed player's slot was not reclaimed"
        assert stop(rejoined) == 0, "Rejoined client leave failed"
        assert stop(replacement) == 0, "Replacement leave failed"
        print("PASS SIGKILL lease expiry permits rejoin", flush=True)

        for fault in ("spoof", "session", "id-zero", "id-high", "replay"):
            proc, path = client(f"fault-{fault}", 6, 30, fault)
            wait_line(path, accepted, proc)
            event = wait_line(server_log, rf"SERVER_DROP identity=6 reason={fault}\b", server)
            if security:
                wait_line(audit, rf"identity=6 event=DROP reason={fault}\b", server)
            assert stop(proc) == 0, f"Fault client {fault} shutdown failed"
            print(f"PASS {event.group(0)}", flush=True)
        for proc, _ in initial[2:]:
            assert proc.poll() is None, "Legitimate client died during faults"
            assert stop(proc) == 0, "Legitimate client shutdown failed"
        assert stop(server) == 0, "Server shutdown failed"
        summary = re.search(r"SERVER_SUMMARY ([^\n]+)", server_log.read_text())
        assert summary, "Missing machine-readable server summary"
        counts = dict((key, int(value)) for key, value in re.findall(r"(\w+)=(\d+)", summary.group(1)))
        for key, minimum in (("joins", 12), ("rejects", 1), ("leaves", 10),
                             ("timeouts", 1), ("invalid", 4), ("stale", 1), ("inputs", 1)):
            assert counts.get(key, -1) >= minimum, f"{key}: expected >= {minimum}, got {counts}"
        assert counts.get("timeouts") == 1, f"Unexpected legitimate-session lease loss: {counts}"
        assert counts.get("peak") == 5, f"Five-player peak/cap missing: {counts}"
        print("SERVER_SUMMARY", summary.group(1), flush=True)
        if security:
            root = re.search(r"AUDIT_ROOT hash=([0-9a-f]{64}) records=(\d+)", server_log.read_text())
            assert root, "Missing audit root"
            count, digest = verify(audit, root.group(1), int(root.group(2)))
            print(f"PASS lifecycle audit records={count} root={digest}", flush=True)
        print("PASS P1 networking lifecycle and malformed-message counters", flush=True)
        return 0
    finally:
        for process in reversed(children):
            if process.poll() is None:
                stop(process)
        for stream in handles:
            stream.close()


if __name__ == "__main__":
    raise SystemExit(main())
