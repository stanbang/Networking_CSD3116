#!/usr/bin/env python3
"""Independently check audit bytes and optional trusted root/count.

Purpose: P2 tamper-evidence verification. Author: <team>/Codex, 2026-11-03.
The expected root must come from a separately retained server exit record.
"""
import argparse
import hashlib
from pathlib import Path
import re


def verify(path, expected_root=None, expected_records=None):
    """Verify exact canonical bytes; raise ValueError on corruption or gaps."""
    pattern = re.compile(
        rb"seq=(\d+) ns=(\d+) identity=(\d+) event=([A-Za-z0-9_-]+) "
        rb"reason=([A-Za-z0-9_-]+) previous=([0-9a-f]{64}) hash=([0-9a-f]{64})\n"
    )
    previous = "0" * 64
    lines = path.read_bytes().splitlines(keepends=True)
    if not lines:
        raise ValueError("Empty audit file")
    for index, line in enumerate(lines, 1):
        match = pattern.fullmatch(line)
        if not match or int(match.group(1)) != index:
            raise ValueError(f"Record {index}: invalid format or sequence")
        if match.group(6).decode() != previous:
            raise ValueError(f"Record {index}: previous hash mismatch")
        computed = hashlib.sha256(line.rsplit(b" hash=", 1)[0]).hexdigest()
        if computed != match.group(7).decode():
            raise ValueError(f"Record {index}: content hash mismatch")
        previous = computed
    if expected_root is not None and previous != expected_root:
        raise ValueError("Final root differs from trusted server root")
    if expected_records is not None and len(lines) != expected_records:
        raise ValueError("Record count differs from trusted server count")
    return len(lines), previous


def main():
    """CLI returns nonzero for any failed audit assertion."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("audit", type=Path)
    parser.add_argument("--expected-root")
    parser.add_argument("--expected-records", type=int)
    args = parser.parse_args()
    count, root = verify(args.audit, args.expected_root, args.expected_records)
    print(f"PASS audit chain records={count} root={root}")


if __name__ == "__main__":
    main()
