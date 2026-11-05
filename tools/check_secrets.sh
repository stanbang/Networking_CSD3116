#!/usr/bin/env bash
# Optional pre-commit check. Author: <team>/Codex, 2026-11-03.
# Human can copy this executable to .git/hooks/pre-commit; no git config needed.
set -euo pipefail
if git ls-files --cached | LC_ALL=C grep -Ei '\.(key|pem|p12|pcap|pcapng|profile|sqlite3(-[^/]*)?)$|(^|/)(accounts\.lock|l[1-6]-[0-9a-f]+\.lock)$'; then
  echo 'Refusing tracked private credentials, account data or packet captures.' >&2
  exit 1
fi
echo 'PASS no tracked private credential, account or capture paths'
