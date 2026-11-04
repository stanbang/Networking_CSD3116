#!/usr/bin/env bash
# Create an isolated deployment bundle. Author: <team>/Codex, 2026-11-03.
# Usage: export_identity.sh PROVISION_DIR server|client1..client6
set -euo pipefail
umask 077
[[ $# == 2 && $2 =~ ^(server|client[1-6])$ ]] || { echo 'Expected provision directory and server|client1..client6' >&2; exit 2; }
SOURCE=$(realpath -- "$1")
[[ $(stat -c %a "$SOURCE") == 700 && $(stat -c %u "$SOURCE") == "$(id -u)" ]] || exit 1
DEST=$(mktemp -d /tmp/dds-participant.XXXXXXXX)
mkdir "$DEST/identity_ca" "$DEST/permissions_ca" "$DEST/identities" "$DEST/permissions"
cp -- "$SOURCE/identity_ca/ca.crt" "$DEST/identity_ca/ca.crt"
cp -- "$SOURCE/permissions_ca/ca.crt" "$DEST/permissions_ca/ca.crt"
cp -- "$SOURCE/governance.p7s" "$DEST/governance.p7s"
cp -- "$SOURCE/identities/$2.crt" "$DEST/identities/$2.crt"
cp -- "$SOURCE/identities/$2.key" "$DEST/identities/$2.key"
cp -- "$SOURCE/permissions/$2.p7s" "$DEST/permissions/$2.p7s"
chmod 600 "$DEST/identities/$2.key"
printf 'SECURITY_DIR=%s\n' "$DEST"
