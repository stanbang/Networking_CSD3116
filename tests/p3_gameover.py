#!/usr/bin/env python3
"""Controlled live collision/game-over/restart proof; not normal-session statistics.

Author: <team>/Codex tester, 2026-11-04. Server scenario prepares collision state;
ordinary simulation must derive the outcome and ordinary encrypted inputs restart.
"""
import argparse
from pathlib import Path
import re
import sys
import time
sys.dont_write_bytecode = True
from p1_network import stop, wait_line
from p2_security import Run
from verify_audit import verify


def main():
    """Require scenario setup, real transitions, observed network state and counters."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--security-dir", required=True)
    parser.add_argument("--bin-dir", default="build/release")
    parser.add_argument("--output-dir", type=Path, default=Path("build/p3-gameover"))
    args = parser.parse_args()
    run = Run(Path(args.bin_dir).resolve(), args.security_dir, args.output_dir)
    audit = (run.out/"audit.log").resolve()
    try:
        server, log = run.start("server", "server", ["--duration", "15", "--test-scenario", "gameover",
                                                      "--audit-file", str(audit)])
        time.sleep(.5)
        clients = [run.start(f"client{i}", "client", ["--identity", str(i), "--headless", "--bot", "hunter",
                                                       "--duration", "10"]) for i in (1,2)]
        for process,path in clients:
            wait_line(path, r"CLIENT_JOIN .*status=accepted", process)
        scenario = wait_line(log, r"SERVER_TEST_SCENARIO name=gameover tick=(\d+)", server)
        defeated = wait_line(log, r"GAME_EVENT gameover tick=(\d+)", server)
        restarted = wait_line(log, r"GAME_EVENT restart tick=(\d+)", server)
        assert int(scenario.group(1)) <= int(defeated.group(1)) < int(restarted.group(1))
        for process,path in clients:
            assert process.wait(timeout=15) == 0, f"Client failed {path}"
        states = [path.read_text() for _,path in clients]
        assert any(re.search(r"CLIENT_GAME_STATE tick=\d+ gameover=1\b", text) for text in states)
        assert all(re.search(r"CLIENT_GAME_STATE tick=\d+ gameover=0\b", text) for text in states)
        assert stop(server) == 0
        text = log.read_text()
        summary = re.search(r"SERVER_SUMMARY ([^\n]+)", text)
        assert summary, "Missing authoritative summary"
        counts = {key:int(value) for key,value in re.findall(r"(\w+)=(\d+)", summary.group(1))}
        assert counts.get("joins")==2 and counts.get("hits",0)>=2, counts
        assert counts.get("gameovers",0)>=1 and counts.get("restarts",0)>=1, counts
        root = re.search(r"AUDIT_ROOT hash=([0-9a-f]{64}) records=(\d+)", text)
        assert root
        verify(audit,root.group(1),int(root.group(2)))
        for event in (scenario,defeated,restarted):
            print(event.group(0),flush=True)
        print("SERVER_SUMMARY",summary.group(1),flush=True)
        print("PASS controlled encrypted collision/game-over/ordinary-input restart",flush=True)
    finally:
        run.close()


if __name__ == "__main__":
    main()
