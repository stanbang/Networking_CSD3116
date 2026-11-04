#!/usr/bin/env bash
# Start a secure two-window local game. Author: <team>/Codex, 2026-11-04.
# Run inside Ubuntu: bash tools/play_local.sh [--headless] [--duration SECONDS]
# A full local provisioning fixture stays in private Linux storage, never the repo.
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
int_duration=3600
b_headless=false
b_fresh=false
str_security_arg=
while (( $# )); do
  case "$1" in
    --headless) b_headless=true; shift ;;
    --security-dir) [[ $# -ge 2 ]] || exit 2; str_security_arg=$2; shift 2 ;;
    --fresh-certs) b_fresh=true; shift ;;
    --duration)
      [[ $# -ge 2 && $2 =~ ^[0-9]{1,5}$ ]] || { echo 'Expected duration 1..86400' >&2; exit 2; }
      int_duration=$((10#$2))
      (( int_duration > 0 && int_duration <= 86400 )) || exit 2
      shift 2 ;;
    *) echo 'Usage: play_local.sh [--headless] [--duration 1..86400] [--fresh-certs]' >&2; exit 2 ;;
  esac
done

# Validate only public certificates and key modes; never read private-key contents.
usable_bundle() {
  local str_root=$1 str_identity str_policy int_expiry
  [[ -d $str_root && ! -L $str_root ]] || return 1
  [[ $(stat -c %a "$str_root") == 700 && $(stat -c %u "$str_root") == "$(id -u)" ]] || return 1
  for str_identity in server client1 client2; do
    [[ -f $str_root/identities/$str_identity.key && ! -L $str_root/identities/$str_identity.key ]] || return 1
    [[ $(stat -c %a "$str_root/identities/$str_identity.key") == 600 ]] || return 1
    openssl x509 -checkend "$int_duration" -noout -in "$str_root/identities/$str_identity.crt" >/dev/null 2>&1 || return 1
    # Grants expire before identity certificates; verify their signature and
    # validity too so an old cached bundle is regenerated before starting DDS.
    str_policy=$(openssl smime -verify -purpose any -in "$str_root/permissions/$str_identity.p7s" \
      -CAfile "$str_root/permissions_ca/ca.crt" 2>/dev/null) || return 1
    [[ $str_policy =~ \<not_after\>([^\<]+)\</not_after\> ]] || return 1
    int_expiry=$(date -d "${BASH_REMATCH[1]}" +%s) || return 1
    (( int_expiry > $(date +%s) + int_duration )) || return 1
  done
}

make -j2
str_security=$str_security_arg
if [[ -z $str_security_arg && $b_fresh == false && -f build/local-security-dir ]]; then
  IFS= read -r str_security < build/local-security-dir || true
fi
if [[ -n $str_security_arg ]] && ! usable_bundle "$str_security"; then
  echo "Explicit security bundle failed checks" >&2; exit 1
fi
if ! usable_bundle "$str_security"; then
  str_result=$(bash tools/gen_certs.sh)
  [[ $str_result == SECURITY_DIR=* ]] || { echo 'Credential generation returned no directory' >&2; exit 1; }
  str_security=${str_result#SECURITY_DIR=}
  usable_bundle "$str_security" || { echo 'Generated credentials failed checks' >&2; exit 1; }
  printf '%s\n' "$str_security" > build/local-security-dir
fi

str_run=$(mktemp -d build/local-session.XXXXXXXX)
# Only this shell's active jobs are signalled; cleanup remains bounded on DDS faults.
cleanup() {
  local -a arr_live=()
  mapfile -t arr_live < <(jobs -pr)
  if (( ${#arr_live[@]} )); then
    kill -TERM "${arr_live[@]}" 2>/dev/null || true
    sleep 1
    mapfile -t arr_live < <(jobs -pr)
    if (( ${#arr_live[@]} )); then kill -KILL "${arr_live[@]}" 2>/dev/null || true; fi
  fi
  wait || true
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
./build/release/server --security-dir "$str_security" --duration "$int_duration" \
  --audit-file "$str_run/audit.log" >"$str_run/server.log" 2>&1 &
int_server=$!
for ((int_attempt=0; int_attempt<200; ++int_attempt)); do
  if grep -q '^SERVER_READY ' "$str_run/server.log" 2>/dev/null; then break; fi
  kill -0 "$int_server" 2>/dev/null || { cat "$str_run/server.log" >&2; exit 1; }
  sleep .1
done
grep -q '^SERVER_READY ' "$str_run/server.log" 2>/dev/null || { echo 'Server startup timed out; see "$str_run/server.log"' >&2; exit 1; }
arr_flags=()
if [[ $b_headless == true ]]; then arr_flags=(--headless --bot circle); fi
./build/release/client --security-dir "$str_security" --identity 1 --duration "$int_duration" \
  "${arr_flags[@]}" >"$str_run/client1.log" 2>&1 &
int_client1=$!
for ((int_attempt=0; int_attempt<200; ++int_attempt)); do
  grep -q 'CLIENT_JOIN .*status=accepted' "$str_run/client1.log" 2>/dev/null && break
  kill -0 "$int_client1" 2>/dev/null || exit 1
  sleep .1
done
grep -q 'CLIENT_JOIN .*status=accepted' "$str_run/client1.log" 2>/dev/null || exit 1
./build/release/client --security-dir "$str_security" --identity 2 --duration "$int_duration" \
  "${arr_flags[@]}" >"$str_run/client2.log" 2>&1 &
int_client2=$!
for ((int_attempt=0; int_attempt<200; ++int_attempt)); do
  grep -q 'CLIENT_JOIN .*status=accepted' "$str_run/client2.log" 2>/dev/null && break
  kill -0 "$int_client2" 2>/dev/null || exit 1
  sleep .1
done
grep -q 'CLIENT_JOIN .*status=accepted' "$str_run/client2.log" 2>/dev/null || exit 1
printf 'Local session started. Logs: %s server=%s client1=%s client2=%s\n' "$str_run" "$int_server" "$int_client1" "$int_client2"
int_status=0
wait -n "$int_server" "$int_client1" "$int_client2" || int_status=$?
if (( int_status )); then echo 'A game process failed; inspect build/local-*.log' >&2; fi
exit "$int_status"
