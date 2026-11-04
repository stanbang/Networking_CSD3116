#!/usr/bin/env python3
"""P0 baseline launcher ownership and recovery. Author: CSD3116 team + AI; 2026-11-04.

This is an explicitly insecure reference demo, never a fallback for the secure game.
The lock serializes launch/stop. pidfds prevent a reused PID receiving a stop signal.
"""
import fcntl
import json
import os
from pathlib import Path
import secrets
import select
import shutil
import signal
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parent.parent
WORK = ROOT / "build" / "p0-play"
STATE = WORK / "session.json"
MARKER = "DDS_ASTEROIDS_P0_SESSION"


def process_start(pid):
    """Read proc start ticks despite spaces/parentheses in the command name."""
    fields = Path(f"/proc/{pid}/stat").read_text().rsplit(")", 1)[1].split()
    return int(fields[19])


def owned_fd(entry, token):
    """Return an fd for an exactly matching owned live process, otherwise None."""
    pid = entry["pid"]
    if not isinstance(pid, int) or pid <= 1:
        raise ValueError("Invalid recorded PID; refusing to signal")
    allowed = {str(ROOT / "build/release/baseline-server"),
               str(ROOT / "build/release/baseline-client")}
    if entry["exe"] not in allowed:
        raise ValueError("Unexpected recorded executable; refusing to signal")
    try:
        fd = os.pidfd_open(pid)
    except ProcessLookupError:
        return None
    try:
        proc = Path(f"/proc/{pid}")
        marker = f"{MARKER}={token}".encode()
        matches = (proc.stat().st_uid == os.getuid()
                   and str((proc / "exe").resolve(strict=True)) == entry["exe"]
                   and process_start(pid) == entry["start"]
                   and os.getsid(pid) == pid
                   and marker in (proc / "environ").read_bytes().split(b"\0"))
        if matches:
            return fd
    except (FileNotFoundError, ProcessLookupError):
        pass
    os.close(fd)
    return None


def write_state(state):
    """Persist each child before starting the next so interruption is recoverable."""
    temporary = WORK / "session.next"
    if temporary.is_symlink() or STATE.is_symlink():
        raise ValueError("Refusing symlinked state")
    temporary.write_text(json.dumps(state, indent=2) + "\n")
    temporary.replace(STATE)


def read_state():
    """Reject corrupt ownership records instead of guessing which process to stop."""
    if STATE.is_symlink():
        raise ValueError("Refusing symlinked state")
    if not STATE.exists():
        return None
    state = json.loads(STATE.read_text())
    if (state.get("phase") != "P0" or not isinstance(state.get("token"), str)
            or not isinstance(state.get("processes"), list)
            or len(state["processes"]) > 3):
        raise ValueError("Invalid P0 state; inspect build/p0-play/session.json")
    return state


def stop_owned(state):
    """Signal only identity-checked pidfds, then bound shutdown to three seconds."""
    if not state:
        print("No recorded P0 session.")
        return
    mismatches = []
    for entry in reversed(state["processes"]):
        fd = owned_fd(entry, state["token"])
        if fd is None:
            print(f"{entry['name']}: exited or ownership mismatch; no signal sent")
            if Path(f"/proc/{entry['pid']}").exists():
                mismatches.append(entry["name"])
            continue
        try:
            signal.pidfd_send_signal(fd, signal.SIGTERM)
            poller = select.poll()
            poller.register(fd, select.POLLIN)
            if not poller.poll(3000):
                signal.pidfd_send_signal(fd, signal.SIGKILL)
                if not poller.poll(1000):
                    raise RuntimeError(f"{entry['name']}: shutdown not confirmed")
            print(f"{entry['name']}: stopped owned P0 process")
        except ProcessLookupError:
            print(f"{entry['name']}: already exited")
        finally:
            os.close(fd)
    if mismatches:
        raise ValueError("Live ownership mismatch; state retained for manual inspection: "
                         + ", ".join(mismatches))


def refuse_existing_baseline():
    """Do not join another manually launched reference server or consume its slots."""
    for proc in Path("/proc").iterdir():
        if not proc.name.isdigit():
            continue
        try:
            target = os.readlink(proc / "exe").removesuffix(" (deleted)")
            if Path(target).name in ("baseline-server", "baseline-client"):
                raise ValueError("An unowned baseline process is already running. "
                                 "Close its own terminal/window before starting P0.")
        except (FileNotFoundError, PermissionError, ProcessLookupError):
            continue


def launch_one(name, executable, state, env, run):
    """Detach a line-buffered child, verify its executable and save its identity."""
    logfile = run / f"{name}.log"
    with logfile.open("xb") as output:
        child = subprocess.Popen(["stdbuf", "-oL", "-eL", str(executable)],
                                 cwd=ROOT, stdin=subprocess.DEVNULL, stdout=output,
                                 stderr=subprocess.STDOUT, env=env, start_new_session=True)
    fd = None
    try:
        fd = os.pidfd_open(child.pid)
        deadline = time.monotonic() + 3
        while time.monotonic() < deadline:
            if child.poll() is not None:
                raise RuntimeError(f"{name} exited: inspect {logfile}")
            if Path(f"/proc/{child.pid}/exe").resolve() == executable:
                break
            time.sleep(0.02)
        else:
            raise RuntimeError(f"{name} failed to execute expected binary")
        entry = {"name": name, "pid": child.pid, "exe": str(executable),
                 "start": process_start(child.pid), "log": str(logfile)}
        state["processes"].append(entry)
        write_state(state)
    except BaseException:
        # This direct child is still unreaped, so its PID cannot be reused.
        try:
            child.terminate()
            child.wait(timeout=3)
        except subprocess.TimeoutExpired:
            child.kill()
            child.wait(timeout=1)
        except ProcessLookupError:
            pass
        raise
    finally:
        if fd is not None:
            os.close(fd)
    return child, logfile


def wait_log(child, logfile, phrases, name):
    """Bound the handshake wait; logs are evidence of protocol progress, not visuals."""
    deadline = time.monotonic() + 20
    while time.monotonic() < deadline:
        text = logfile.read_text(errors="replace")
        if child.poll() is not None:
            raise RuntimeError(f"{name} exited; inspect {logfile}")
        if "Unable to join" in text or "Failed to create participant" in text:
            raise RuntimeError(f"{name} failed; inspect {logfile}")
        if all(phrase in text for phrase in phrases):
            # The baseline prints the response before checking its ID. Allow the
            # adjacent rejection line to flush; the GUI still verifies player IDs.
            time.sleep(0.15)
            if "Unable to join" in logfile.read_text(errors="replace"):
                raise RuntimeError(f"{name} rejected; inspect {logfile}")
            return
        time.sleep(0.05)
    raise RuntimeError(f"{name} timed out; inspect {logfile}")


def play():
    """Launch server, first joined client, then second joined client for P0 only."""
    if not os.environ.get("DISPLAY"):
        raise ValueError("DISPLAY is unset. Open a WSLg terminal and retry.")
    if os.environ.get("CYCLONEDDS_URI"):
        raise ValueError("P0 expects default DDS domain 0; unset CYCLONEDDS_URI first.")
    if not shutil.which("stdbuf"):
        raise ValueError("Missing stdbuf; install coreutils.")
    server = ROOT / "build/release/baseline-server"
    client = ROOT / "build/release/baseline-client"
    for executable in (server, client):
        if not executable.is_file() or not os.access(executable, os.X_OK):
            raise ValueError("Baseline executable missing; run make baseline first.")
    previous = read_state()
    if previous:
        for entry in previous["processes"]:
            fd = owned_fd(entry, previous["token"])
            if fd is not None:
                os.close(fd)
                raise ValueError("P0 already running; use bash tools/stop.sh first.")
            if Path(f"/proc/{entry['pid']}").exists():
                raise ValueError("Live ownership mismatch; inspect saved P0 state first.")
    refuse_existing_baseline()
    token = secrets.token_hex(16)
    run = WORK / (time.strftime("%Y%m%d-%H%M%S") + "-" + token[:8])
    run.mkdir(mode=0o700)
    state = {"phase": "P0", "token": token, "processes": []}
    env = dict(os.environ, DDS_ASTEROIDS_P0_SESSION=token)
    write_state(state)
    try:
        child, log = launch_one("server", server, state, env, run)
        wait_log(child, log, ("Waiting...",), "server")
        for name in ("client1", "client2"):
            child, log = launch_one(name, client, state, env, run)
            wait_log(child, log, ("Join response received", "Position received"), name)
        for entry in state["processes"]:
            fd = owned_fd(entry, token)
            if fd is None:
                raise RuntimeError(f"{entry['name']} exited before startup completed")
            os.close(fd)
        print("P0 baseline launched: server and two clients report protocol activity.")
        print("Traffic is PLAINTEXT; this is the reference demo, not the secure game.")
        print("Visually check two windows, distinct Player 1/2 labels and movement.")
        print(f"Logs: {run}")
        print(f"Recovery: cd '{ROOT}' && bash tools/stop.sh")
    except BaseException:
        stop_owned(state)
        raise


def main():
    """Serialize all lifecycle operations and retain state for audit/recovery."""
    if len(sys.argv) != 2 or sys.argv[1] not in ("play", "stop"):
        raise ValueError("Usage: bash tools/play.sh | bash tools/stop.sh (P0 only)")
    if not hasattr(os, "pidfd_open") or not hasattr(signal, "pidfd_send_signal"):
        raise ValueError("Safe stop requires Linux pidfds and Python 3.9+.")
    WORK.mkdir(mode=0o700, parents=True, exist_ok=True)
    if WORK.resolve() != WORK or (WORK / "lock").is_symlink():
        raise ValueError("Refusing symlinked runtime directory or lock")
    with (WORK / "lock").open("a") as lock:
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError as error:
            raise ValueError("Another P0 launch/stop is in progress") from error
        if sys.argv[1] == "play":
            play()
        else:
            stop_owned(read_state())


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, RuntimeError, KeyError) as error:
        print(f"P0: {error}", file=sys.stderr)
        sys.exit(1)
