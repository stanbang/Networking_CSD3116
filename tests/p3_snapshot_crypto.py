#!/usr/bin/env python3
"""Observe protected server egress during five-client gameplay, without mutation.

Author: <team>/Codex tester, 2026-11-04. Socket observation, not external sniffing.
Transform-kind values2/4 are AES128/256-GCM in installed dds_security_types.h.
The encrypted packet alone does not expose its opaque topic identity.
"""
import argparse
import os
from pathlib import Path
import re
import sys
import time
sys.dont_write_bytecode = True
from p1_network import stop, wait_line
from p2_security import Run


def transformation(data):
    """Parse bounded public RTPS security headers independently of the shim."""
    assert data[:4] == b"RTPS"
    position = 20
    while position + 4 <= len(data):
        kind, flags = data[position:position+2]
        length = int.from_bytes(data[position+2:position+4], "little" if flags & 1 else "big")
        assert length and position+4+length <= len(data)
        if kind in (0x31,0x33):
            assert length >= 8
            return int.from_bytes(data[position+4:position+8], "big")
        position += 4+length
    raise AssertionError("No public native crypto prefix")


def main():
    """Capture only after all five clients receive real authoritative gameplay."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--security-dir",required=True)
    parser.add_argument("--bin-dir",default="build/release")
    args=parser.parse_args()
    run=Run(Path(args.bin_dir).resolve(),args.security_dir,Path("build/p3-snapshot-crypto"))
    arm=(run.out/"arm").resolve(); arm.unlink(missing_ok=True)
    capture=(run.out/"server-egress").resolve()
    for suffix in ("before","after"):
        Path(str(capture)+f"-{suffix}.bin").unlink(missing_ok=True)
    env=dict(os.environ,LD_PRELOAD=str(Path("build/rtps_fault.so").resolve()),AST_FAULT_ARM=str(arm),
             AST_FAULT_CAPTURE=str(capture),AST_FAULT_OBSERVE="1")
    try:
        server,log=run.start("server","server",["--duration","12","--audit-file",str(run.out/"audit.log")],env)
        time.sleep(.5)
        bots=[run.start(f"client{i}","client",["--identity",str(i),"--headless","--bot","hunter",
                                                "--duration","8"]) for i in range(1,6)]
        for process,path in bots:
            wait_line(path,r"CLIENT_SNAPSHOT tick=\d+",process)
        offset=log.stat().st_size
        arm.touch(); time.sleep(2); arm.unlink()
        wait_line(log,r"WIRE_OBSERVE offset=\d+ bytes=\d+ send_result=\d+",server)
        for process,_ in bots:
            assert process.wait(timeout=12)==0
        assert stop(server)==0
        before=Path(str(capture)+"-before.bin").read_bytes()
        after=Path(str(capture)+"-after.bin").read_bytes()
        assert before==after, "Passive observer must preserve every outgoing byte"
        kind=transformation(before)
        assert kind in (2,4), f"Not AES-GCM encryption: transformkind={kind}"
        tail=log.read_bytes()[offset:].decode(errors="replace")
        assert "SERVER_JOIN" not in tail, "Capture interval included another admission"
        assert "mutations=0" in tail
        counts=dict((key,int(value)) for key,value in re.findall(r"(\w+)=(\d+)",
                    re.search(r"SERVER_SUMMARY ([^\n]+)",tail).group(1)))
        assert counts["snapshots"]>50 and counts["joins"]==5 and counts["peak"]==5
        print(f"PASS passive server egress: transformkind={kind} AES-GCM bytes={len(before)} unchanged=1",flush=True)
        print(f"PASS concurrent live snapshots={counts['snapshots']} fiveclients=5",flush=True)
        print("Scope: protected server socket egress during Snapshot workload; opaque topic not decoded.",flush=True)
    finally:
        arm.unlink(missing_ok=True)
        run.close()


if __name__=="__main__":
    main()
