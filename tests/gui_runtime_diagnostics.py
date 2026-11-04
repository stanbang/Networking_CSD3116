#!/usr/bin/env python3
"""Observe an unchanged GUI test; CSD3116 team + AI assistance, 2026-11-04.

No signals, keys or application state are changed here. The default is pure
observation; explicit --dds-markers preloads only native game children.
CPU/RSS samples are every0.5s; textual thread wait/CPU groups every2s.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import threading
import time


def proc_stat(path):
    """Return CPU/start ticks without printing pointers or command arguments."""
    fields = path.read_text().rsplit(')', 1)[1].split()
    return int(fields[11]) + int(fields[12]), int(fields[19])


def thread_groups(proc):
    """Aggregate only textual thread names/waits and cumulative CPU ticks."""
    waits, cpu = {}, {}
    for task in (proc / 'task').iterdir():
        try:
            name = (task / 'comm').read_text().strip()[:64]
            if name.startswith('llvmpipe-'):
                name = 'llvmpipe-*'
            ticks, _ = proc_stat(task / 'stat')
            cpu[name] = cpu.get(name, 0) + ticks
            wait = (task / 'wchan').read_text().strip()
            if wait != '0' and not wait.isidentifier():
                wait = 'unavailable'
            waits[wait] = waits.get(wait, 0) + 1
        except (OSError, ValueError, IndexError):
            continue
    return dict(thread_waits=waits, thread_cpu_ticks=cpu)


def sample(entry, detailed):
    """Read one owned child's public scheduling metadata, refusing PID reuse."""
    result = dict(role=entry['role'], pid=entry['pid'])
    proc = Path('/proc') / str(entry['pid'])
    try:
        ticks, start = proc_stat(proc / 'stat')
        if start != entry['start']:
            return dict(result, state='exited_or_reused')
        result.update(state='present', cpu_ticks=ticks)
        for line in (proc / 'status').read_text().splitlines():
            key, _, value = line.partition(':')
            if key in ('Threads', 'VmRSS', 'VmHWM'):
                result[key] = int(value.split()[0])
        if detailed:
            result.update(thread_groups(proc))
    except (OSError, ValueError, IndexError):
        result['state'] = 'unavailable_or_exited'
    return result


def main():
    """Run the existing test with read-only observation of its exact children."""
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--diagnostics', type=Path, required=True)
    parser.add_argument('--dds-markers', type=Path)
    parser.add_argument('--suite', choices=['gui', 'network'], default='gui')
    args, remaining = parser.parse_known_args()
    marker = args.dds_markers.resolve(strict=True) if args.dds_markers else None
    if marker and not marker.is_file():
        raise ValueError('DDS marker must be an existing regular file')
    # Both existing acceptance modules retain their own assertions and behavior.
    if args.suite == 'network':
        import p1_network as acceptance
    else:
        import p3_gui as acceptance
    args.diagnostics.parent.mkdir(parents=True, exist_ok=True)
    stream = args.diagnostics.open('x')
    entries, lock, stop = [], threading.Lock(), threading.Event()
    original = subprocess.Popen

    def emit(record):
        with lock:
            stream.write(json.dumps(dict(observed_monotonic=time.monotonic(), **record)) + '\n')
            stream.flush()

    def observed_popen(*positional, **kwargs):
        command = positional[0] if positional else kwargs.get('args', [])
        game = isinstance(command, (list, tuple)) and command and Path(str(command[0])).name in ('server', 'client')
        if game and marker:
            child_env = dict(os.environ if kwargs.get('env') is None else kwargs['env'])
            if child_env.get('LD_PRELOAD'):
                raise ValueError('Conflicting LD_PRELOAD; refusing to mix DDS instrumentation')
            child_env['LD_PRELOAD'] = str(marker)
            kwargs['env'] = child_env
        child = original(*positional, **kwargs)
        if game:
            try:
                _, start = proc_stat(Path(f'/proc/{child.pid}/stat'))
                entry = dict(role=Path(str(command[0])).name, pid=child.pid, start=start)
                with lock:
                    entries.append(entry)
                emit(dict(event='child_started', **entry))
            except (OSError, ValueError, IndexError):
                emit(dict(event='child_exited_before_sample', pid=child.pid))
        return child

    def observe():
        count = 0
        while not stop.is_set():
            with lock:
                current = list(entries)
            record = dict(event='sample', processes=[sample(entry, count % 4 == 0) for entry in current])
            emit(record)
            count += 1
            stop.wait(.5)

    emit(dict(event='metadata', clock_ticks_per_second=os.sysconf('SC_CLK_TCK'),
              sample_seconds=.5, thread_detail_seconds=2, dds_markers=str(marker) if marker else None,
              LP_NUM_THREADS=os.environ.get('LP_NUM_THREADS'), suite=args.suite))
    thread = threading.Thread(target=observe, name='gui-readonly-observer', daemon=True)
    subprocess.Popen = observed_popen
    thread.start()
    original_argv = sys.argv
    try:
        sys.argv = [acceptance.__name__ + '.py', *remaining]
        return acceptance.main()
    finally:
        subprocess.Popen = original
        sys.argv = original_argv
        stop.set()
        thread.join(timeout=3)
        if not thread.is_alive():
            emit(dict(event='observer_stopped'))
            stream.close()
        else:
            print('Diagnostic observer did not finish in3s; no additional signals sent.', file=sys.stderr)


if __name__ == '__main__':
    raise SystemExit(main())
