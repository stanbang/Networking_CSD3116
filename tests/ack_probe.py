#!/usr/bin/env python3
"""Isolate one identity6 Leave/ACK with server kept alive. Team/AI, 2026-11-05.

Diagnostic only: production ACK timeout, policies, wire and lease are unchanged.
No private material or raw DDS trace is requested. Only owned children are stopped.
"""
import argparse
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import time
from account_fixture import AccountFixture


def wait_for(path, pattern, child, deadline, seconds=8):
    """Bound startup observations; never include arbitrary child output in errors."""
    until = min(deadline, time.monotonic() + seconds)
    while time.monotonic() < until:
        match = re.search(pattern, path.read_text(errors='replace'), re.M)
        if match:
            return match
        if child.poll() is not None:
            raise RuntimeError(path.name + ': child exited before expected event')
        time.sleep(.05)
    raise RuntimeError(path.name + ': expected event not observed before deadline')


def stop_owned(child, events):
    """Signal only a still-owned unreaped child and reap with finite bounds."""
    if child.poll() is None:
        events.append(dict(event='cleanup_signal', pid=child.pid, monotonic_ns=time.monotonic_ns()))
        try:
            child.terminate()
        except ProcessLookupError:
            pass
    try:
        return child.wait(timeout=5)
    except subprocess.TimeoutExpired:
        child.kill()
        child.wait(timeout=3)
        events.append(dict(event='cleanup_forced_kill', pid=child.pid))
        return child.returncode


def exercise(args, start, result, events):
    """Keep the server alive across exactly one normal-timed client ACK attempt."""
    deadline = time.monotonic() + 20
    server, server_log = start('server', ['--duration', '60', '--audit-file', str(args.output_dir / 'audit.log')])
    wait_for(server_log, r'^SERVER_READY ', server, deadline)
    events.append(dict(event='server_ready_observed', monotonic_ns=time.monotonic_ns()))
    client, client_log = start('client', ['--identity', '6', '--headless', '--bot', 'idle',
                                        '--fault', 'spoof', '--duration', '30'])
    match = wait_for(client_log, r'^CLIENT_JOIN .* id=([1-5]) status=accepted$', client, deadline)
    result['assigned_id'] = int(match.group(1))
    wait_for(server_log, r'^SERVER_DROP identity=6 reason=spoof$', server, deadline, 3)
    wait_for(client_log, r'^CLIENT_SNAPSHOT tick=\d+ id=' + match.group(1) + r' ', client, deadline, 3)
    if server.poll() is not None or client.poll() is not None:
        raise RuntimeError('A peer exited before the controlled Leave request')
    events.append(dict(event='client_sigterm_requested', monotonic_ns=time.monotonic_ns(),
                       realtime_ns=time.time_ns(), server_alive=True))
    client.send_signal(signal.SIGTERM)
    try:
        result['client_exit'] = client.wait(timeout=5)
    except subprocess.TimeoutExpired:
        raise RuntimeError('Client did not finish unchanged ACK/teardown within five seconds')
    result['server_alive_after_client_exit'] = server.poll() is None
    events.append(dict(event='client_exit_observed', monotonic_ns=time.monotonic_ns(),
                       realtime_ns=time.time_ns(), code=client.returncode,
                       server_alive=result['server_alive_after_client_exit']))
    time.sleep(2)
    result['server_alive_two_seconds_later'] = server.poll() is None
    result['server_leave_observed'] = bool(re.search(r'^SERVER_LEAVE identity=6 id=[1-5]$',
                                                       server_log.read_text(), re.M))
    audit = args.output_dir / 'audit.log'
    result['audit_leave_observed'] = bool(re.search(r'identity=6 event=LEAVE reason=explicit\b',
                                                      audit.read_text()))


def main():
    """Validate fresh paths and preserve actual outputs even when ACK acceptance fails."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--security-dir', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--bin-dir', type=Path, default=Path('build/release'))
    parser.add_argument('--dds-markers', type=Path)
    args = parser.parse_args()
    if os.environ.get('LD_PRELOAD'):
        parser.error('Unset inherited LD_PRELOAD; only explicit game-child markers are allowed')
    bundle = args.security_dir.resolve(strict=True)
    if bundle.stat().st_uid != os.getuid() or bundle.stat().st_mode & 0o777 != 0o700:
        parser.error('Private Linux credential directory must be owned and mode0700')
    marker = args.dds_markers.resolve(strict=True) if args.dds_markers else None
    if marker and not marker.is_file():
        parser.error('Marker must be an existing regular file')
    args.output_dir = args.output_dir.resolve()
    args.output_dir.mkdir(parents=True, exist_ok=False)
    binaries = args.bin_dir.resolve()
    children, streams, events = [], [], []
    accounts = AccountFixture()
    result = dict(scope='isolated single-identity Leave/ACK diagnostic',
                  server_duration_seconds=60, client_duration_seconds=30,
                  production_ack_timeout_seconds=1, events=events)

    def start(kind, extra):
        path = args.output_dir / (kind + '.log')
        stream = path.open('x')
        streams.append(stream)
        env = dict(os.environ)
        env.pop('LP_NUM_THREADS', None)
        env.pop('CYCLONEDDS_URI', None)
        if marker:
            env['LD_PRELOAD'] = str(marker)
        command = [str(binaries / kind), '--security-dir', str(bundle), *extra]
        print('COMMAND ' + ' '.join(command), flush=True)
        child = accounts.spawn(command, stdout=stream, stderr=subprocess.STDOUT, env=env)
        children.append((kind, child))
        events.append(dict(event='child_started', role=kind, pid=child.pid,
                           monotonic_ns=time.monotonic_ns()))
        return child, path

    try:
        exercise(args, start, result, events)
    except (OSError, RuntimeError) as error:
        result['error'] = str(error)
    finally:
        for kind, child in reversed(children):
            result[kind + '_cleanup_exit'] = stop_owned(child, events)
        for stream in streams:
            stream.close()
    server_log = args.output_dir / 'server.log'
    result['server_summary_observed'] = server_log.exists() and 'SERVER_SUMMARY ' in server_log.read_text()
    passed = (not result.get('error') and result.get('client_exit') == 0
              and result.get('server_cleanup_exit') == 0 and result.get('server_summary_observed')
              and all(result.get(field) for field in ('server_alive_after_client_exit',
                  'server_alive_two_seconds_later', 'server_leave_observed', 'audit_leave_observed')))
    result['status'] = 'PASS' if passed else 'FAIL'
    payload = json.dumps(result, indent=2)
    (args.output_dir / 'summary.json').write_text(payload + '\n')
    print(payload, flush=True)
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
