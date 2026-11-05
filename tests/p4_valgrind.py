#!/usr/bin/env python3
"""Independent secured server/five-client Memcheck run.

Author: <team>/Codex tester, 2026-11-04. Treat invalid accesses and definite
leaks as failures; retain all raw Memcheck output, including other leak classes.
"""
import argparse
from pathlib import Path
import re
import shlex
import subprocess
import sys
import time
sys.dont_write_bytecode=True
from p1_network import stop,wait_line


def inspect_logs(memlogs,logs):
    """Retain and inspect every process/Memcheck log, including failed runs."""
    failures=[]
    for path in logs:
        try:
            text=path.read_text(errors="replace")
            print("LOG_REVIEW",path,f"bytes={path.stat().st_size}",flush=True)
            for line in text.splitlines():
                if any(label in line for label in ("SERVER_SUMMARY", "CLIENT_SUMMARY", "leave_ack:", "ERROR", "FATAL")):
                    print(line,flush=True)
        except OSError as error:
            print("LOG_REVIEW",path,"unreadable",str(error),flush=True)
            failures.append(str(path))
    for path in memlogs:
        try:
            text=path.read_text(errors="replace")
        except OSError as error:
            print("LOG_REVIEW",path,"unreadable",str(error),flush=True)
            failures.append(str(path))
            continue
        errors=re.search(r"ERROR SUMMARY: ([\d,]+) errors",text)
        definite=re.search(r"definitely lost: ([\d,]+) bytes",text)
        clean_errors=errors and int(errors.group(1).replace(",",""))==0
        clean_leaks=(definite and int(definite.group(1).replace(",",""))==0) or "no leaks are possible" in text
        print("LOG_REVIEW",path,f"zero_errors={bool(clean_errors)} zero_definite_leaks={bool(clean_leaks)}",flush=True)
        for line in text.splitlines():
            if any(label in line for label in ("definitely lost:","indirectly lost:","possibly lost:","still reachable:","ERROR SUMMARY:")):
                print(line,flush=True)
        if not (clean_errors and clean_leaks):failures.append(str(path))
    return failures


from account_fixture import AccountFixture


def main():
    """Finite workload and bounded cleanup; errors never become success by filtering."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--security-dir",required=True)
    parser.add_argument("--bin-dir",type=Path,default=Path("build/release"),
                        help="Directory containing the server and client under test")
    parser.add_argument("--output-dir",type=Path,default=Path("build/p4-valgrind"),
                        help="New evidence directory; existing output is never overwritten")
    args=parser.parse_args()
    out=args.output_dir;out.mkdir(parents=True,exist_ok=False)
    children=[];streams=[];memlogs=[];logs=[]
    accounts = AccountFixture()

    def start(name,kind,extra):
        path=out/(name+".log");memlog=(out/(name+"-memcheck.log")).resolve()
        stream=path.open("x");streams.append(stream);memlogs.append(memlog);logs.append(path)
        command=["valgrind","--leak-check=full","--show-leak-kinds=all","--errors-for-leak-kinds=definite",
                 "--track-origins=yes","--error-exitcode=99","--log-file="+str(memlog),
                 str(args.bin_dir.resolve()/kind),"--security-dir",args.security_dir,*extra]
        print("COMMAND",shlex.join(command),flush=True)
        process=accounts.spawn(command,stdout=stream,stderr=subprocess.STDOUT)
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
    finally:
        for process in reversed(children):
            if process.poll() is None:
                try:stop(process)
                except AssertionError as error:print("CLEANUP",error,file=sys.stderr)
        for stream in streams:stream.close()
        failures=inspect_logs(memlogs,logs)
        if failures:
            print("FAIL Memcheck or unreadable/incomplete logs:",failures,file=sys.stderr,flush=True)
            if sys.exc_info()[0] is None:raise AssertionError(f"Memcheck log failures: {failures}")
    print("PASS secured server + five clients under Valgrind",flush=True)


if __name__=="__main__":main()
