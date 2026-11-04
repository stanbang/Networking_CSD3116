#!/usr/bin/env bash
# Optional pre-commit check. Author: <team>/Codex, 2026-11-03.
# Human can copy this executable to .git/hooks/pre-commit; no git config needed.
set -euo pipefail
if git ls-files --cached | LC_ALL=C grep -Ei '\.(key|pem)$'; then
  echo 'Refusing tracked private-key/PEM paths; keep runtime credentials outside the repo.' >&2
  exit 1
fi
echo 'PASS no tracked .key/.pem paths'
