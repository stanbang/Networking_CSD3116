#!/usr/bin/env bash
# Secure game by default; explicit --baseline preserves P0. CSD3116 team/AI, 2026-11-04.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
if [[ ${1:-} == --baseline ]]; then
  shift
  exec python3 -B "$ROOT/tools/p0_session.py" stop "$@"
fi
exec python3 -B "$ROOT/tools/game_session.py" stop "$@"
