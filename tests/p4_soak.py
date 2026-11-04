#!/usr/bin/env python3
"""Bounded secure five-client churn soak with sanitizer log checks.

Author: <team>/Codex tester, 2026-11-04. Finite child commands, seeded churn,
graceful leaves and intentional SIGKILL lease reclamation; no global changes.
"""
import argparse
from pathlib import Path
import random
import re
import signal
import subprocess
import sys
import time
sys.dont_write_bytecode=True
from p1_network import stop,wait_line
from verify_audit import verify


class Soak:
    """Own all logs/processes, and require real successful joins and shutdowns."""
    def __init__(self,args):
        self.args=args;self.out=Path("build/p4-"+Path(args.bin_dir).name+"-soak")
        self.out.mkdir(parents=True,exist_ok=True)
        self.children=[];self.streams=[];self.logs=[];self.sessions=[0]*5

    def start(self,name,kind,extra):
        """Keep sanitizer stderr with per-process stdout for independent inspection."""
        path=self.out/(name+".log"); stream=path.open("w")
        self.streams.append(stream);self.logs.append(path)
        command=[str(Path(self.args.bin_dir).resolve()/kind),"--security-dir",self.args.security_dir,*extra]
        if self.args.setarch:
            command=["setarch","x86_64","-R",*command]
        process=subprocess.Popen(command,stdout=stream,stderr=subprocess.STDOUT)
        self.children.append(process)
        return process,path

    def client(self,identity):
        """Use real certificates and ordinary hunter inputs for gameplay during churn."""
        self.sessions[identity-1]+=1
        process,path=self.start(f"client{identity}-session{self.sessions[identity-1]}","client",
                                ["--identity",str(identity),"--headless","--bot","hunter",
                                 "--duration",str(self.args.duration+30)])
        wait_line(path,r"CLIENT_JOIN .*status=accepted",process,timeout=12)
        return process,path

    def cleanup(self):
        """Always attempt every owned child even if one fails bounded shutdown."""
        for process in reversed(self.children):
            if process.poll() is None:
                try:stop(process)
                except AssertionError as error:print("CLEANUP",error,file=sys.stderr)
        for stream in self.streams:stream.close()


def churn(run,server,clients):
    """Keep five identities admitted between bounded random churn events."""
    rng=random.Random(run.args.seed);begin=time.monotonic();events=kills=graceful=0
    while time.monotonic()-begin < run.args.duration:
        remaining=run.args.duration-(time.monotonic()-begin)
        time.sleep(min(rng.uniform(3,7),max(0,remaining)))
        assert server.poll() is None and all(p.poll() is None for p,_ in clients)
        if time.monotonic()-begin >= run.args.duration:break
        index=rng.randrange(5);killed=rng.random()<.25
        code=stop(clients[index][0],signal.SIGKILL if killed else signal.SIGTERM)
        assert code==(-signal.SIGKILL if killed else 0),f"Unexpected client exit {code}"
        kills+=killed;graceful+=not killed;events+=1
        time.sleep(3.4 if killed else .25)
        clients[index]=run.client(index+1)
        print(f"CHURN elapsed={time.monotonic()-begin:.1f}s identity={index+1} killed={int(killed)} events={events}",flush=True)
    elapsed=time.monotonic()-begin
    assert elapsed>=run.args.duration and events>=5 and kills>=1 and graceful>=1
    for process,_ in clients:assert stop(process)==0
    assert stop(server)==0
    return elapsed,events,kills,graceful


def inspect(run,server_log,audit,elapsed,events,kills,graceful):
    """Scan all process logs, inspect live effects and anchor the audit hash chain."""
    failures=[]
    for path in run.logs:
        text=path.read_text(errors="replace")
        if re.search(r"ERROR: (?:AddressSanitizer|LeakSanitizer)|AddressSanitizer:DEADLYSIGNAL|runtime error:|WARNING: ThreadSanitizer|FATAL: ThreadSanitizer",text):
            failures.append(str(path))
    assert not failures,f"Sanitizer diagnostic logs: {failures}"
    text=server_log.read_text();summary=re.search(r"SERVER_SUMMARY ([^\n]+)",text)
    assert summary
    counts={key:int(value) for key,value in re.findall(r"(\w+)=(\d+)",summary.group(1))}
    assert counts["joins"]==5+events and counts["peak"]==5,counts
    assert counts["timeouts"]>=kills and counts["leaves"]>=graceful+5,counts
    assert counts["ticks"]>=run.args.duration*30,counts
    for key in ("moves","shots","destroyed","snapshots"):assert counts[key]>0,counts
    root=re.search(r"AUDIT_ROOT hash=([0-9a-f]{64}) records=(\d+)",text);assert root
    verify(audit,root.group(1),int(root.group(2)))
    print("SERVER_SUMMARY",summary.group(1),flush=True)
    print(f"PASS secure soak elapsed={elapsed:.1f}s churn={events} kills={kills} graceful={graceful} logs={len(run.logs)}",flush=True)
    print("PASS no sanitizer diagnostics in owned process logs; final audit verified",flush=True)


def main():
    """Minimum120seconds required; a120-second result is not a ten-minute course soak."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--security-dir",required=True);parser.add_argument("--bin-dir",default="build/asan")
    parser.add_argument("--duration",type=int,default=120);parser.add_argument("--seed",type=int,default=3116)
    parser.add_argument("--setarch",action="store_true",help="Disable ASLR only for these test processes (TSan mapping workaround)")
    args=parser.parse_args();assert 120<=args.duration<=600
    run=Soak(args);audit=(run.out/"audit.log").resolve()
    try:
        server,log=run.start("server","server",["--duration",str(args.duration+40),"--audit-file",str(audit)])
        wait_line(log,r"SERVER_READY security=NATIVE_DDS",server,timeout=12)
        clients=[run.client(identity) for identity in range(1,6)]
        inspect(run,log,audit,*churn(run,server,clients))
    finally:run.cleanup()


if __name__=="__main__":main()
