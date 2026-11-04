#!/usr/bin/env bash
# Fail-closed ELF protection assertions. Author: <team>/Codex, 2026-11-04.
# GNU readelf is authoritative; do not infer protections from compiler flags alone.
set -euo pipefail
export LC_ALL=C
if [[ $# == 0 ]]; then echo 'Usage: check_hardening.sh ELF [ELF ...]' >&2; exit 2; fi
command -v readelf >/dev/null || { echo 'readelf missing (human: sudo apt install binutils)' >&2; exit 2; }

# Every required property must be present. Each command failure returns nonzero.
verify_file() {
  local binary=$1 header program dynamic symbols references
  [[ -f $binary && -r $binary ]] || { echo "FAIL $binary missing/unreadable" >&2; return 1; }
  header=$(readelf -hW -- "$binary") || return 1
  program=$(readelf -lW -- "$binary") || return 1
  dynamic=$(readelf -dW -- "$binary") || return 1
  symbols=$(readelf -sW -- "$binary") || return 1
  if ! grep -Eq '^[[:space:]]*Type:[[:space:]]+DYN([[:space:]]|$)' <<< "$header" ||
     ! grep -Eq '\(FLAGS_1\).*([[:space:]]|,)PIE([[:space:],]|$)' <<< "$dynamic"; then
    echo "FAIL $binary PIE: requires DYN and dynamic PIE flag" >&2; return 1
  fi
  if ! awk '$1=="GNU_STACK" {seen++; for(i=7;i<NF;i++) if(index($i,"E")) executable=1} END {exit !(seen==1&&!executable)}' <<< "$program"; then
    echo "FAIL $binary NX: requires one non-executable GNU_STACK" >&2; return 1
  fi
  if ! awk '$1=="GNU_RELRO" {found=1} END {exit !found}' <<< "$program"; then
    echo "FAIL $binary RELRO: GNU_RELRO segment absent" >&2; return 1
  fi
  if ! grep -Eq '\(BIND_NOW\)|\(FLAGS\).*BIND_NOW|\(FLAGS_1\).*Flags:.*[[:space:]]NOW([[:space:],]|$)' <<< "$dynamic"; then
    echo "FAIL $binary full RELRO: BIND_NOW/NOW absent" >&2; return 1
  fi
  references=$(awk '$7=="UND" && $8 ~ /^__stack_chk_fail(@|$)/ {count++} END {print count+0}' <<< "$symbols")
  if [[ $references == 0 ]]; then echo "FAIL $binary canary: no __stack_chk_fail reference" >&2; return 1; fi
  echo "== $binary"
  grep -E '^[[:space:]]*Type:' <<< "$header"
  grep -E '^[[:space:]]*GNU_STACK|^[[:space:]]*GNU_RELRO' <<< "$program"
  grep -E '\(BIND_NOW\)|\(FLAGS\)|\(FLAGS_1\)' <<< "$dynamic"
  echo "PASS PIE NX RELRO BIND_NOW CANARY references=$references"
}

# Check both executables even if the first fails, and preserve any failure.
result=0
for binary in "$@"; do
  if verify_file "$binary"; then :; else result=1; fi
done
exit "$result"
