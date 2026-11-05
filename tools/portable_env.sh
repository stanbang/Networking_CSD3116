#!/usr/bin/env bash
## @file portable_env.sh
# @brief Set library and executable paths for the bundled Ubuntu SDK.
# Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
# Project AI-use disclosure: docs/AI_USAGE.md.

# Local Ubuntu24.04 x86-64 SDK environment. Team/Codex2026-11-05.
SDK="$ROOT/portable/linux-x86_64"
[[ $(uname -m) == x86_64 ]] || { echo 'This ZIP targets x86_64 Ubuntu24.04 under WSL.' >&2; return 1; }
[[ -f "$SDK/manifest.json" ]] || { echo 'Bundled SDK missing; use the complete offline source ZIP.' >&2; return 1; }
chmod u+x "$SDK/bin/openssl" "$SDK/bin/python3"
export PATH="$SDK/bin:$PATH"
export LD_LIBRARY_PATH="$SDK/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export PYTHONHOME="$SDK" PYTHONNOUSERSITE=1
unset PYTHONPATH
export OPENSSL_CONF="$SDK/openssl.cnf"
export LIBGL_DRIVERS_PATH="$SDK/lib/dri"
export LIBGL_ALWAYS_SOFTWARE="${LIBGL_ALWAYS_SOFTWARE:-1}"
export LP_NUM_THREADS="${LP_NUM_THREADS:-2}"
