#!/usr/bin/env bash
# Unzip on Ubuntu24.04 x86-64/WSLg with GCC available; compile and start locally.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
source "$ROOT/tools/portable_env.sh"
exec bash "$ROOT/tools/play_local.sh" "$@"
