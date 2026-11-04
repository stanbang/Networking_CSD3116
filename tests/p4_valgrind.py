#!/usr/bin/env python3
"""Independent secured server/five-client Memcheck run.

Author: <team>/Codex tester, 2026-11-04. Treat invalid accesses and definite
leaks as failures; retain all raw Memcheck output, including other leak classes.
"""
import argparse
from pathlib import Path
import re
import subprocess
import sys
import time
sys.dont_write_bytecode=True
from p1_network import stop,wait_line


def main():
    """Finite workload and bounded cleanup; errors never become success by filtering."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--security-dir",required=True)
    args=parser.parse_args()
    out=Path("build/p4-valgrind");out.mkdir(parents=True,exist_ok=True)
    children=[];streams=[];memlogs=[]

    def start(name,kind,extra):
        path=out/(name+".log");memlog=(out/(name+"-memcheck.log")).resolve()
        stream=path.open("w");streams.append(stream);memlogs.append(memlog)
        command=["valgrind","--leak-check=full","--show-leak-kinds=all","--errors-for-leak-kinds=definite",
                 "--track-origins=yes","--error-exitcode=99","--log-file="+str(memlog),
                 str(Path("build/release").resolve()/kind),"--security-dir",args.security_dir,*extra]
        process=subprocess.Popen(command,stdout=stream,stderr=subprocess.STDOUT)
        children.append(process)
        return process,path

    try:
        server,server_log=start("server","server",["--duration","45","--audit-file",str(out/"audit.log")])
        wait_line(server_log,r"SERVER_READY security=NATIVE_DDS",server,timeout=25)
        clients=[start(f"client{i}","client",["--identity",str(i),"--headless","--bot","hunter",
                                               "--duration","20"]) for i in range(1,6)]
        for process,path in clients:wait_line(path,r"CLIENT_JOIN .*status=accepted",process,timeout=20)
        for process,_ in clients:assert process.wait(timeout=30)==0,"Valgrind client failure"
        assert stop(server)==0,"Valgrind server failure"
        for path in memlogs:
            text=path.read_text()
            errors=re.search(r"ERROR SUMMARY: ([\d,]+) errors",text)
            assert errors and int(errors.group(1).replace(",",""))==0,path
            definite=re.search(r"definitely lost: ([\d,]+) bytes",text)
            assert (definite and int(definite.group(1).replace(",",""))==0) or "no leaks are possible" in text,path
            print(f"PASS {path.name}: zero Memcheck errors, zero definitely-lost bytes",flush=True)
            for line in text.splitlines():
                if any(label in line for label in ("definitely lost:","indirectly lost:","possibly lost:","still reachable:","ERROR SUMMARY:")):
                    print(line,flush=True)
        print("PASS secured server + five clients under Valgrind",flush=True)
    finally:
        for process in reversed(children):
            if process.poll() is None:
                try:stop(process)
                except AssertionError as error:print("CLEANUP",error,file=sys.stderr)
        for stream in streams:stream.close()


if __name__=="__main__":main()
