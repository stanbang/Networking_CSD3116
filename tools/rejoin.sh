#!/usr/bin/env bash
## @file rejoin.sh
# @brief Reopen one client against the existing local server.
# Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
# Project AI-use disclosure: docs/AI_USAGE.md.

# Reopen one participant against the existing local server. Team/Codex,2026-11-05.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
cd -- "$ROOT"
source "$ROOT/tools/portable_env.sh"
[[ $# -ge 1 && $1 =~ ^[1-6]$ ]] || { echo 'Usage: bash tools/rejoin.sh PLAYER_NUMBER [client options]' >&2; exit 2; }
int_identity=$1
shift
str_cache=build/local-active-security-dir
[[ -f $str_cache ]] || str_cache=build/local-security-dir
[[ -f $str_cache && -x build/release/client ]] || { echo 'Start RunGame.cmd first. Reconnect does not build or start another server.' >&2; exit 1; }
IFS= read -r str_security < "$str_cache"
[[ -d $str_security ]] || { echo 'Local credentials are unavailable. Start a new game with RunGame.cmd.' >&2; exit 1; }
printf 'Reopening player %s. Sign in to the SAME account before the 30-second grace expires.\n' "$int_identity"
exec ./build/release/client --security-dir "$str_security" --identity "$int_identity" --duration 3600 "$@"
