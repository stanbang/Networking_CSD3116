#!/usr/bin/env bash
# Generate two private CAs (identity + permissions) and identity certs for the server and N clients,
# using the OpenSSL workflow from the Week 10 lecture (genrsa -> CSR -> CA-signed X.509).
# Usage: bash tools/gen_certs.sh [outdir=security/generated] [num_clients=5]
# Output is gitignored. NEVER commit *.key files.
set -euo pipefail
OUT="${1:-security/generated}"
N="${2:-5}"
DAYS=30
umask 077
mkdir -p "$OUT"

make_ca() {  # $1 = dir, $2 = common name
  mkdir -p "$OUT/$1"
  openssl genrsa -out "$OUT/$1/ca.key" 2048
  openssl req -new -x509 -sha256 -key "$OUT/$1/ca.key" -out "$OUT/$1/ca.crt" \
    -days "$DAYS" -subj "/C=SG/O=SIT/OU=ICT/CN=$2"
}

issue() {    # $1 = CA dir, $2 = identity name
  local d="$OUT/identities"; mkdir -p "$d"
  openssl genrsa -out "$d/$2.key" 2048
  openssl req -new -key "$d/$2.key" -out "$d/$2.csr" -subj "/C=SG/O=SIT/OU=ICT/CN=$2"
  openssl x509 -req -sha256 -in "$d/$2.csr" -CA "$OUT/$1/ca.crt" -CAkey "$OUT/$1/ca.key" \
    -CAcreateserial -out "$d/$2.crt" -days "$DAYS"
  rm -f "$d/$2.csr"
}

make_ca identity_ca    "DDS-Asteroids-Identity-CA"
make_ca permissions_ca "DDS-Asteroids-Permissions-CA"
issue identity_ca server
for i in $(seq 1 "$N"); do issue identity_ca "client$i"; done

echo "Done. Inspect a cert with: openssl x509 -in $OUT/identities/server.crt -text -noout"
echo "Signed governance/permissions XML (S/MIME) is generated separately - see docs/PROMPT.md section 5."
