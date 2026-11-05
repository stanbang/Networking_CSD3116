#!/usr/bin/env bash
# One-shot environment setup for Ubuntu / WSL2 (Ubuntu). Run: bash tools/setup_env.sh
set -euo pipefail

sudo apt update
sudo apt install -y build-essential pkg-config git gdb valgrind \
  openssl libssl-dev libsqlite3-dev \
  libgl1-mesa-dev libglfw3-dev \
  cyclonedds-dev cyclonedds-tools

echo; echo "=== Environment checks ==="
command -v gcc >/dev/null && gcc --version | head -1
command -v idlc >/dev/null && echo "idlc: OK" || echo "idlc: MISSING (needed to generate messages.c/h)"
echo "CycloneDDS: $(pkg-config --modversion CycloneDDS 2>/dev/null || echo MISSING)"
echo "GLFW:       $(pkg-config --modversion glfw3 2>/dev/null || echo MISSING)"
echo "SQLite:     $(pkg-config --modversion sqlite3 2>/dev/null || echo MISSING)"
openssl version

echo; echo "=== DDS Security plugins (needed for the preferred security design) ==="
if find /usr /opt -name 'libdds_security_*' 2>/dev/null | grep -q .; then
  find /usr /opt -name 'libdds_security_*' 2>/dev/null
else
  echo "NOT FOUND -> the apt CycloneDDS build probably has no security plugins."
  echo "Either build CycloneDDS from source with -DENABLE_SECURITY=ON (see README)"
  echo "or use the application-layer security fallback described in docs/PROMPT.md."
fi

echo; echo "=== Display / platform ==="
echo "DISPLAY=${DISPLAY:-unset}  WAYLAND_DISPLAY=${WAYLAND_DISPLAY:-unset}"
if grep -qi microsoft /proc/version; then
  echo "WSL detected: GUI windows need WSLg (Windows 11 / recent Windows 10)."
  echo "'perf' does NOT work in WSL - use the course Linux server over SSH (short runs only)."
fi
echo "ASLR (want 2): $(cat /proc/sys/kernel/randomize_va_space)"
