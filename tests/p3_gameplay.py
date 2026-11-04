#!/usr/bin/env python3
"""Observe secure five-bot gameplay from public summaries/snapshots.

Author: <team>/Codex tester, 2026-11-04. Direct deterministic sim fixtures cover
game-over/restart; --require-gameover additionally demands them in this live run.
"""
import argparse
from pathlib import Path
import re
import subprocess
import sys
import time
sys.dont_write_bytecode = True
from p1_network import stop, wait_line


def check_client(path, process):
    """Demand finite successful gameplay observations, not merely a join exit code."""
    assert process.wait(timeout=10) == 0, f"Bot failed: {path}"
    text = path.read_text()
    rows = [dict((key, int(value)) for key, value in re.findall(r"(\w+)=(-?\d+)", row))
            for row in re.findall(r"CLIENT_SNAPSHOT [^\n]+", text)]
    assert len(rows) >= 5, f"Too few authoritative snapshots: {path}: {text}"
    assert len({row["tick"] for row in rows}) >= 5, f"Frozen snapshot tick: {path}"
    assert len({(row["x"], row["y"]) for row in rows}) > 1, f"Ship never moved: {path}"
    assert all(0 <= row["x"] < 80000 and 0 <= row["y"] < 60000 for row in rows)
    assert all(row["vx"]**2 + row["vy"]**2 <= 20002**2 for row in rows), "Observed speed exceeds 200 px/s cap"
    print(f"PASS {path.name}: snapshots={len(rows)} maxscore={max(row['score'] for row in rows)} "
          f"minlives={min(row['lives'] for row in rows)}", flush=True)
    return rows


def check_server(path, require_gameover):
    """Demand real simulation effects independently of the client log assertions."""
    text = path.read_text()
    summary = re.search(r"SERVER_SUMMARY ([^\n]+)", text)
    assert summary, "Missing authoritative summary"
    counts = {key: int(value) for key, value in re.findall(r"(\w+)=(\d+)", summary.group(1))}
    for key in ("moves", "shots", "destroyed", "splits", "hits", "respawns", "wraps", "snapshots"):
        assert counts.get(key, 0) > 0, f"No observed {key}: {counts}"
    assert counts.get("peak") == 5 and counts.get("joins") == 5, counts
    if require_gameover:
        assert counts.get("gameovers", 0) > 0 and counts.get("restarts", 0) > 0, counts
    print("SERVER_SUMMARY", summary.group(1), flush=True)
    print("PASS authoritative gameplay effects", flush=True)


def main():
    """Run finite secured bots; preserve logs and stop only owned processes."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--security-dir", required=True)
    parser.add_argument("--bin-dir", default="build/release")
    parser.add_argument("--duration", type=int, default=35)
    parser.add_argument("--bot", default="hunter")
    parser.add_argument("--require-gameover", action="store_true")
    args = parser.parse_args()
    assert 10 <= args.duration <= 300, "Use a bounded meaningful duration"
    out = Path("build/p3-gameplay")
    out.mkdir(parents=True, exist_ok=True)
    children, streams = [], []

    def start(name, kind, extra):
        path = out / f"{name}.log"
        stream = path.open("w")
        streams.append(stream)
        process = subprocess.Popen([str(Path(args.bin_dir).resolve()/kind), "--security-dir",
                                    args.security_dir, *extra], stdout=stream, stderr=subprocess.STDOUT)
        children.append(process)
        return process, path

    try:
        server, server_log = start("server", "server", ["--duration", str(args.duration+8),
                                                        "--audit-file", str(out/"audit.log")])
        time.sleep(.5)
        bots = [start(f"client{i}", "client", ["--identity", str(i), "--headless", "--bot", args.bot,
                                               "--duration", str(args.duration)]) for i in range(1,6)]
        for process, path in bots:
            wait_line(path, r"CLIENT_JOIN .*status=accepted", process)
        time.sleep(max(0, args.duration-2))
        snapshots = [row for process,path in bots for row in check_client(path,process)]
        assert any(row["score"] > 0 for row in snapshots), "No positive score received"
        assert any(row["lives"] < 3 for row in snapshots), "No damage/life change received"
        assert stop(server) == 0, "Authoritative server failed"
        check_server(server_log, args.require_gameover)
    finally:
        for process in reversed(children):
            if process.poll() is None:
                try:
                    stop(process)
                except AssertionError as error:
                    print(f"CLEANUP {error}", file=sys.stderr)
        for stream in streams:
            stream.close()


if __name__ == "__main__":
    main()
