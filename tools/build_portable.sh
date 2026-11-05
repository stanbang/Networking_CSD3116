#!/usr/bin/env bash
# Offline source build: host GCC, bundled headers/libs/generated DDS. No apt/make/idlc/pkg-config.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
cd -- "$ROOT"
source "$ROOT/tools/portable_env.sh"
command -v cc >/dev/null || { echo 'A GCC C toolchain must already be available.' >&2; exit 1; }
sha256sum -c portable/generated/idl.sha256 >/dev/null
mkdir -p build/release
flags=(-std=gnu11 -O2 -Wall -Wextra -Wformat=2 -Wformat-security -fstack-protector-strong -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Isrc -Iportable/generated -isystem third_party -isystem "$SDK/include" "-DAST_PLUGIN_DIR=\"$SDK/lib/\"")
link=(-pie -Wl,-z,relro,-z,now,-z,noexecstack "-L$SDK/lib" '-Wl,-rpath,$ORIGIN/../../portable/linux-x86_64/lib' -lddsc -lssl -lcrypto -pthread)
common=(src/common/*.c portable/generated/messages.c portable/generated/accounts.c)
printf '[1/2] Compiling server...\n'
cc "${flags[@]}" src/server/*.c "${common[@]}" -o build/release/server "${link[@]}" -lsqlite3 -lm
printf '[2/2] Compiling client...\n'
cc "${flags[@]}" src/client/*.c "${common[@]}" -o build/release/client "${link[@]}" -lglfw -lGL -lm
printf 'Offline source build complete. No packages installed.\n'
