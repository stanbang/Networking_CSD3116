#!/usr/bin/env python3
"""Owned secure launcher supervisor. CSD3116 team + declared AI assistance, 2026-11-04.

The contributor shell owns/reaps only its direct children. This helper records
and signals only that exact supervisor, using pidfds to avoid PID reuse races.
"""
import argparse
import fcntl
import json
import os
from pathlib import Path
import secrets
import select
import signal
import subprocess
import time

ROOT = Path(__file__).resolve().parent.parent
WORK = ROOT / 'build/game-play'
STATE = WORK / 'session.json'
MARKER = 'DDS_ASTEROIDS_SECURE_SESSION'
BASH = Path('/usr/bin/bash').resolve()


def start_ticks(pid):
    """Extract start ticks without assuming the process name contains no spaces."""
    return int(Path(f'/proc/{pid}/stat').read_text().rsplit(')', 1)[1].split()[19])


def owned_fd(state):
    """Return a pidfd only for the exact owned supervisor, never a reused PID."""
    pid = state['pid']
    if not isinstance(pid, int) or pid <= 1:
        raise ValueError('Invalid supervisor PID')
    try:
        fd = os.pidfd_open(pid)
    except ProcessLookupError:
        return None
    try:
        proc = Path(f'/proc/{pid}')
        args = (proc / 'cmdline').read_bytes().split(b'\0')
        marker = f"{MARKER}={state['token']}".encode()
        if (proc.stat().st_uid == os.getuid() and os.readlink(proc / 'exe') == str(BASH)
                and start_ticks(pid) == state['start'] and os.getsid(pid) == pid
                and str(ROOT / 'tools/play_local.sh').encode() in args
                and marker in (proc / 'environ').read_bytes().split(b'\0')):
            return fd
    except (OSError, ProcessLookupError):
        pass
    os.close(fd)
    if Path(f'/proc/{pid}').exists():
        raise ValueError('Live ownership mismatch; refusing to signal or overwrite state')
    return None


def stop(state):
    """Ask the owned shell to clean up its children, then verify bounded exit."""
    if not state:
        print('No recorded secure session.')
        return
    fd = owned_fd(state)
    if fd is None:
        print('Recorded secure session has exited.')
        return
    try:
        signal.pidfd_send_signal(fd, signal.SIGTERM)
        poll = select.poll()
        poll.register(fd, select.POLLIN)
        if not poll.poll(8000):
            raise RuntimeError('Supervisor cleanup timed out; state retained, no broad kill sent')
        print('Stopped owned secure session.')
    finally:
        os.close(fd)


def play(args, previous):
    """Launch the secure session and wait for both login windows or headless admissions."""
    if previous:
        fd = owned_fd(previous)
        if fd is not None:
            os.close(fd)
            raise ValueError('Secure session already running; use tools/stop.sh')
    if not args.headless and not os.environ.get('DISPLAY'):
        raise ValueError('DISPLAY missing; run from WSLg or use --headless')
    bundle = Path(args.security_dir).resolve(strict=True)
    if bundle.stat().st_uid != os.getuid() or bundle.stat().st_mode & 0o777 != 0o700:
        raise ValueError('Security bundle must be private owner-only Linux storage')
    targets = {str(ROOT / 'build/release/server'), str(ROOT / 'build/release/client')}
    for proc in Path('/proc').iterdir():
        if not proc.name.isdigit():
            continue
        try:
            if os.readlink(proc / 'exe').removesuffix(' (deleted)') in targets:
                raise ValueError('An unowned game process is running; close its own session first')
        except (FileNotFoundError, PermissionError, ProcessLookupError):
            continue
    token = secrets.token_hex(16)
    log = WORK / (token[:12] + '.log')
    cmd = [str(BASH), str(ROOT / 'tools/play_local.sh'), '--security-dir', str(bundle),
           '--duration', str(args.duration)]
    if args.headless:
        cmd.append('--headless')
    with log.open('xb') as stream:
        child = subprocess.Popen(cmd, cwd=ROOT, stdin=subprocess.DEVNULL, stdout=stream,
                                 stderr=subprocess.STDOUT, start_new_session=True,
                                 env=dict(os.environ, **{MARKER: token}))
    state = dict(pid=child.pid, start=start_ticks(child.pid), token=token, log=str(log))
    temporary = WORK / 'session.next'
    if temporary.is_symlink() or STATE.is_symlink():
        child.terminate()
        raise ValueError('Refusing symlinked session state')
    temporary.write_text(json.dumps(state) + '\n')
    temporary.replace(STATE)
    try:
        deadline = time.monotonic() + 70
        while time.monotonic() < deadline:
            if child.poll() is not None:
                raise RuntimeError('Secure launcher exited; inspect ' + str(log))
            text = log.read_text(errors='replace')
            if 'Local session started.' in text:
                print(text.strip())
                print('SECURE_SESSION_READY; confirm actual windows separately.')
                return
            time.sleep(0.1)
        raise RuntimeError('Secure startup timed out; inspect ' + str(log))
    except BaseException:
        stop(state)
        raise


def main():
    """Serialize play/stop without touching the baseline session or unrelated jobs."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['play', 'stop'])
    parser.add_argument('--security-dir')
    parser.add_argument('--duration', type=int, default=3600)
    parser.add_argument('--headless', action='store_true')
    args = parser.parse_args()
    if args.action == 'play' and (not args.security_dir or not 1 <= args.duration <= 86400):
        parser.error('play requires --security-dir and duration1..86400')
    WORK.mkdir(parents=True, exist_ok=True)
    if WORK.resolve() != WORK or (WORK / 'lock').is_symlink() or STATE.is_symlink():
        raise ValueError('Refusing symlinked runtime path')
    with (WORK / 'lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        previous = json.loads(STATE.read_text()) if STATE.exists() else None
        if args.action == 'stop':
            stop(previous)
        else:
            play(args, previous)


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, RuntimeError, KeyError) as error:
        raise SystemExit(str(error))
