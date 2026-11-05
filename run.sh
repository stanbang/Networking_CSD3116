#!/usr/bin/env bash
## @file run.sh
# @brief Load the bundled SDK and start a local DDS Asteroids session.
# Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
# Project AI-use disclosure: docs/AI_USAGE.md.

# Unzip on Ubuntu24.04 x86-64/WSLg with GCC available; compile and start locally.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
source "$ROOT/tools/portable_env.sh"
exec bash "$ROOT/tools/play_local.sh" "$@"
