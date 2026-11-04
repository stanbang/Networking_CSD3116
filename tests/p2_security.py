#!/usr/bin/env python3
"""P2 native security black-box tests. Author: <team>/Codex, 2026-11-03.

Keys stay in the supplied private Linux --security-dir; stdout never prints keys.
Native authentication/access diagnostics are retained as evidence, never invented.
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
from verify_audit import verify


class Run:
    """Own finite children and log files, cleaning up even after assertions."""
    def __init__(self, binaries, security, out):
        self.binaries, self.security, self.out = binaries, security, out
        self.children, self.files = [], []
        out.mkdir(parents=True, exist_ok=True)

    def start(self, name, kind, extra, env=None):
        """Launch one secured process with bounded test-provided duration."""
        command = [str(self.binaries / kind), "--security-dir", self.security, *extra]
        path = self.out / f"{name}.log"
        stream = path.open("w")
        self.files.append(stream)
        print("COMMAND", " ".join(command), flush=True)
        process = subprocess.Popen(command, stdout=stream, stderr=subprocess.STDOUT, env=env)
        self.children.append(process)
        return process, path

    def client(self, name, fault=None, duration="3", env=None):
        """Identity6 is reserved for negative cases; identity1 stays legitimate."""
        flags = ["--identity", "6", "--headless", "--bot", "circle", "--duration", duration]
        if fault:
            flags += ["--fault", fault]
        return self.start(name, "client", flags, env)

    def close(self):
        """Terminate only owned children; preserve evidence on disk."""
        errors = []
        for process in reversed(self.children):
            if process.poll() is None:
                try:
                    stop(process)
                except AssertionError as error:
                    errors.append(str(error))
        for stream in self.files:
            stream.close()
        if errors:
            print("CLEANUP_ERRORS", errors, file=sys.stderr)


def audit_delta(path, offset, pattern, server, timeout=4):
    """Require new correlated records; prior cases cannot satisfy this assertion."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        content = path.read_bytes()[offset:].decode(errors="replace")
        match = re.search(pattern, content)
        if match:
            print(f"PASS new audit event {match.group(0)}", flush=True)
            return
        assert server.poll() is None, "Server died before audit rejection"
        time.sleep(.05)
    raise AssertionError(f"Missing new audit record {pattern!r}: {content}")


def denial_cases(run, server, legitimate, audit):
    """Do not conflate absence of application join with a middleware audit event."""
    for fault in ("no-cert", "wrong-ca", "rogue-server"):
        offset = audit.stat().st_size
        process, path = run.client(fault, fault)
        code = process.wait(timeout=15)
        output = path.read_text(errors="replace")
        assert not re.search(r"CLIENT_JOIN .*status=accepted", output), fault
        new_records = audit.read_bytes()[offset:].decode(errors="replace")
        assert not re.search(r"identity=6 event=JOIN ", new_records), f"Unauthorized slot allocation: {new_records}"
        assert server.poll() is None and legitimate.poll() is None, "Healthy processes died"
        if fault == "rogue-server":
            assert code != 0, "Forbidden authoritative writer must fail"
            assert "CLIENT_FORBIDDEN_WRITER result=denied" in output, output
        if fault == "wrong-ca":
            audit_delta(audit, offset, r"identity=0 event=DDS_AUTH_WARNING reason=authentication\b", server)
        elif fault == "no-cert":
            audit_delta(audit, offset, r"identity=0 event=DDS_UNAUTHENTICATED_DISCOVERY reason=nonsecure-spdp\b", server)
        elif fault == "rogue-server":
            # Local DDS access control refuses the writer before any forbidden
            # sample can reach the server. Claim the genuine local denial only.
            assert not re.search(r"identity=6 event=JOIN ", new_records), new_records
        print(f"PASS {fault}: no accepted join, exit={code}; see native diagnostics in {path}", flush=True)


def audit_cases(audit, server_output, out):
    """Validate against server root, then require tamper/truncation detection."""
    root = re.search(r"AUDIT_ROOT hash=([0-9a-f]{64}) records=(\d+)", server_output)
    assert root, "No final audit root"
    count, digest = verify(audit, root.group(1), int(root.group(2)))
    print(f"PASS audit chain records={count} root={digest}", flush=True)
    raw = audit.read_bytes()
    changed = bytearray(raw)
    position = raw.index(b"reason=") + len(b"reason=")
    changed[position] = ord("X") if changed[position] != ord("X") else ord("Y")
    for label, content in (("modified", changed), ("truncated", raw[:raw.rfind(b"\n", 0, -1)+1])):
        path = out / f"audit-{label}.log"
        path.write_bytes(content)
        try:
            verify(path, root.group(1), int(root.group(2)))
        except ValueError as error:
            print(f"PASS audit {label} detected: {error}", flush=True)
        else:
            raise AssertionError(f"Audit {label} was not detected")


def backpressure_case(run, server, server_log, legitimate):
    """An authenticated non-reading peer must not terminate healthy players."""
    process, path = run.client("slow-reader", "slow-reader", "12")
    code = process.wait(timeout=20)
    assert code in (0, 1), f"Slow reader abnormal exit {code}"
    # Responses deliberately retain only the latest idempotent result. Exceed
    # the former queue bound without relying on a timeout that should not occur.
    replies = len(re.findall(r"SERVER_JOIN identity=6 ", server_log.read_text()))
    assert replies > 32, f"Slow peer did not exceed old queue capacity: {replies}"
    assert server.poll() is None and legitimate.poll() is None
    print(f"PASS slow reader: {replies} responses; legitimate player and server survive", flush=True)
    time.sleep(3.5)
    process, path = run.client("after-slow-reader", duration="2")
    wait_line(path, r"CLIENT_JOIN .*status=accepted", process)
    assert process.wait(timeout=8) == 0, "Fresh client could not recover after slow reader"
    print("PASS fresh authenticated join after slow-reader departure", flush=True)


def wire_case(run, server, server_log, legitimate, audit):
    """Mutate already encrypted RTPS bytes, proving original sendmsg succeeded."""
    arm = (run.out / "wire-arm").resolve()
    arm.unlink(missing_ok=True)
    capture = (run.out / "wire").resolve()
    for suffix in ("before", "after"):
        Path(str(capture) + f"-{suffix}.bin").unlink(missing_ok=True)
    environment = dict(os.environ, LD_PRELOAD=str(Path("build/rtps_fault.so").resolve()),
                       AST_FAULT_ARM=str(arm), AST_FAULT_CAPTURE=str(capture))
    process, path = run.client("wire-tamper", duration="5", env=environment)
    wait_line(path, r"CLIENT_JOIN .*status=accepted", process)
    before_drops = server_log.read_text().count("NATIVE_GCM_AUTH_FAILURE")
    offset = audit.stat().st_size
    arm.touch()
    time.sleep(1)
    arm.unlink()
    assert process.wait(timeout=12) == 0, "Tamper client failed unexpectedly"
    output = path.read_text()
    match = re.search(r"WIRE_FAULT offset=(\d+) bytes=(\d+) send_result=(\d+)", output)
    assert match and match.group(2) == match.group(3), "No successful mutated socket send"
    before = Path(str(capture) + "-before.bin").read_bytes()
    after = Path(str(capture) + "-after.bin").read_bytes()
    assert len(before) == len(after) == int(match.group(2))
    differences = [i for i, (a, b) in enumerate(zip(before, after)) if a != b]
    assert differences == [int(match.group(1))], "Mutation changed unexpected packet bytes"
    assert before[differences[0]] ^ after[differences[0]] == 1
    assert server.poll() is None and legitimate.poll() is None, "Healthy processes died"
    after_drops = server_log.read_text().count("NATIVE_GCM_AUTH_FAILURE")
    assert after_drops > before_drops, "No actual receiver GCM finalization failure after mutation"
    audit_delta(audit, offset, r"identity=0 event=DDS_DECODE_WARNING reason=decode\b", server)
    print(f"PASS actual ciphertext send: byte={differences[0]} packet_bytes={len(after)}", flush=True)
    print(f"PASS receiver OpenSSL GCM authentication failure observed", flush=True)


def main():
    """Exercise security negatives with a persistent honest client, then audit."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--security-dir", required=True)
    parser.add_argument("--bin-dir", default="build/release")
    args = parser.parse_args()
    run = Run(Path(args.bin_dir).resolve(), args.security_dir, Path("build/p2-test"))
    audit = (run.out / "server-audit.log").resolve()
    try:
        environment = dict(os.environ, LD_PRELOAD=str(Path("build/rtps_fault.so").resolve()))
        server, server_log = run.start("server", "server", ["--duration", "90", "--audit-file", str(audit)], environment)
        time.sleep(.5)
        legitimate, path = run.start("legitimate", "client", ["--identity", "1", "--headless",
                                                               "--duration", "80", "--bot", "circle"])
        legitimate_log = path
        wait_line(path, r"CLIENT_JOIN .*status=accepted", legitimate)
        denial_cases(run, server, legitimate, audit)
        backpressure_case(run, server, server_log, legitimate)
        for fault in ("flood", "join-flood", "leave-flood"):
            offset = audit.stat().st_size
            ticks_before = [int(value) for value in re.findall(r"CLIENT_SNAPSHOT tick=(\d+)", legitimate_log.read_text())]
            tick_before = max(ticks_before, default=0)
            process, path = run.client(fault, fault)
            wait_line(path, r"CLIENT_JOIN .*status=accepted", process)
            assert process.wait(timeout=12) == 0, f"{fault} process failed"
            kind = {"join-flood": "join", "leave-flood": "leave"}.get(fault, "input")
            event = wait_line(server_log, rf"SERVER_RATE identity=6 kind={kind}\b", server)
            audit_delta(audit, offset, rf"identity=6 event=RATE reason={kind}\b", server)
            ticks_after = [int(value) for value in re.findall(r"CLIENT_SNAPSHOT tick=(\d+)", legitimate_log.read_text())]
            assert max(ticks_after, default=0) > tick_before, "Honest client's gameplay snapshots stalled during flood"
            velocities = [(int(x), int(y)) for x,y in re.findall(r" vx=(-?\d+) vy=(-?\d+)", path.read_text())]
            assert velocities and all(x*x+y*y <= 20002**2 for x,y in velocities), "Flood client exceeded observed speed cap"
            print(f"PASS {event.group(0)}", flush=True)
            if fault == "leave-flood":
                # The abusive identity can exhaust its own Leave bucket, so its
                # final legitimate Leave may be dropped. Permit the documented
                # lease to release that session before reusing its identity.
                time.sleep(3.5)
        wire_case(run, server, server_log, legitimate, audit)
        assert stop(legitimate) == 0
        assert stop(server) == 0
        summary = re.search(r"SERVER_SUMMARY ([^\n]+)", server_log.read_text())
        assert summary, "Missing final server summary"
        counts = {key: int(value) for key, value in re.findall(r"(\w+)=(\d+)", summary.group(1))}
        assert counts.get("moves", 0) > 0 and counts.get("ticks", 0) > 0, counts
        assert counts["moves"] <= counts["ticks"] * 5, "Movement updated faster than authoritative ticks"
        assert counts.get("rate_hits", 0) > 0, counts
        print("PASS flood tick bound and honest snapshot progress", flush=True)
        audit_cases(audit, server_log.read_text(), run.out)
        print("PASS P2 harness assertions; native rejection/rate details require report evidence", flush=True)
    finally:
        run.close()


if __name__ == "__main__":
    main()
