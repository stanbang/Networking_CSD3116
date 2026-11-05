#!/usr/bin/env bash
## @file gen_certs.sh
# @brief Generate private DDS credentials and signed policies for local play.
# Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
# Project AI-use disclosure: docs/AI_USAGE.md.

# Private runtime CA/identity/policy material. Author: <team>/Codex, 2026-11-03.
# Usage: bash tools/gen_certs.sh [NEW_PRIVATE_LINUX_DIRECTORY]
# No key contents are printed. Never place this output in a source archive.
set -euo pipefail
umask 077
if [[ $# -gt 1 ]]; then echo 'Usage: gen_certs.sh [new_private_linux_directory]' >&2; exit 2; fi
if [[ $# -eq 1 ]]; then mkdir -m 700 -- "$1"; OUT=$(realpath -- "$1");
else OUT=$(mktemp -d /tmp/dds-asteroids.XXXXXXXX); fi
[[ $(stat -c %a "$OUT") == 700 && $(stat -c %u "$OUT") == "$(id -u)" ]] || {
  echo 'Refusing filesystem that cannot enforce private mode0700' >&2; exit 1;
}
mkdir "$OUT/identities" "$OUT/permissions"

# RSA2048/SHA256 matches course constraints. CA keys stay in this private bundle.
make_ca() {
  mkdir "$OUT/$1"
  openssl genrsa -out "$OUT/$1/ca.key" 2048
  openssl req -new -x509 -sha256 -key "$OUT/$1/ca.key" -out "$OUT/$1/ca.crt" \
    -days 30 -subj "/C=SG/O=SIT/OU=ICT/CN=$2" \
    -addext 'basicConstraints=critical,CA:TRUE' -addext 'keyUsage=critical,digitalSignature,keyCertSign,cRLSign'
}

# $1 CA directory, $2 filename, $3 certificate common name.
issue() {
  openssl genrsa -out "$OUT/identities/$2.key" 2048
  openssl req -new -key "$OUT/identities/$2.key" -out "$OUT/identities/$2.csr" -subj "/C=SG/O=SIT/OU=ICT/CN=$3"
  openssl x509 -req -sha256 -in "$OUT/identities/$2.csr" -CA "$OUT/$1/ca.crt" -CAkey "$OUT/$1/ca.key" \
    -CAcreateserial -out "$OUT/identities/$2.crt" -days 30
  rm -- "$OUT/identities/$2.csr"
}

# S/MIME signed-data; XML and signature contain public policy, never a private key.
sign_policy() {
  openssl smime -sign -md sha256 -in "$1" -text -out "$2" \
    -signer "$OUT/permissions_ca/ca.crt" -inkey "$OUT/permissions_ca/ca.key"
}

# Enumerate exact topics: wildcard topic permissions would break identity binding.
topics() {
  local role=$1 operation=$2 identity=$3 i kind
  if [[ $role == server ]]; then
    for i in {1..6}; do
      if [[ $operation == publish ]]; then
        for kind in Response AccountResponse; do echo "<topic>DDSAsteroids_${kind}_$i</topic>"; done
      else for kind in Join Input Leave AccountRequest; do echo "<topic>DDSAsteroids_${kind}_$i</topic>"; done; fi
    done
  elif [[ $operation == publish ]]; then
    for kind in Join Input Leave AccountRequest; do echo "<topic>DDSAsteroids_${kind}_$identity</topic>"; done
  else for kind in Response AccountResponse; do echo "<topic>DDSAsteroids_${kind}_$identity</topic>"; done; fi
  if [[ $operation == publish && $role == server || $operation == subscribe && $role == client ]]; then
    echo '<topic>DDSAsteroids_Snapshot_0</topic>'
  fi
}

# Bind a grant to the certificate's canonical subject and a short validity period.
permissions() {
  local role=$1 name=$2 identity=$3 subject before after xml
  subject=$(openssl x509 -in "$OUT/identities/$name.crt" -noout -subject -nameopt RFC2253)
  subject=${subject#subject=}
  before=$(date -u -d '1 day ago' +%Y-%m-%dT%H:%M:%SZ)
  after=$(date -u -d '29 days' +%Y-%m-%dT%H:%M:%SZ)
  xml="$OUT/permissions/$name.xml"
  cat > "$xml" <<XML
<dds><permissions><grant name="$name">
<subject_name>$subject</subject_name>
<validity><not_before>$before</not_before><not_after>$after</not_after></validity>
<allow_rule><domains><id>42</id></domains>
<publish><topics>$(topics "$role" publish "$identity")</topics><partitions><partition>*</partition></partitions></publish>
<subscribe><topics>$(topics "$role" subscribe "$identity")</topics><partitions><partition>*</partition></partitions></subscribe>
</allow_rule><default>DENY</default></grant></permissions></dds>
XML
  sign_policy "$xml" "$OUT/permissions/$name.p7s"
}

make_ca identity_ca DDS-Asteroids-Identity-CA
make_ca permissions_ca DDS-Asteroids-Permissions-CA
make_ca rogue_ca DDS-Asteroids-Untrusted-Test-CA
issue identity_ca server server
for i in {1..6}; do issue identity_ca "client$i" "client$i"; done
issue rogue_ca rogue_client6 client6
cat > "$OUT/governance.xml" <<'XML'
<dds><domain_access_rules><domain_rule><domains><id>42</id></domains>
<allow_unauthenticated_participants>false</allow_unauthenticated_participants>
<enable_join_access_control>true</enable_join_access_control>
<discovery_protection_kind>ENCRYPT_WITH_ORIGIN_AUTHENTICATION</discovery_protection_kind>
<liveliness_protection_kind>ENCRYPT_WITH_ORIGIN_AUTHENTICATION</liveliness_protection_kind>
<rtps_protection_kind>ENCRYPT_WITH_ORIGIN_AUTHENTICATION</rtps_protection_kind>
<topic_access_rules><topic_rule><topic_expression>*</topic_expression>
<enable_discovery_protection>true</enable_discovery_protection>
<enable_liveliness_protection>true</enable_liveliness_protection>
<enable_read_access_control>true</enable_read_access_control>
<enable_write_access_control>true</enable_write_access_control>
<metadata_protection_kind>ENCRYPT_WITH_ORIGIN_AUTHENTICATION</metadata_protection_kind>
<data_protection_kind>ENCRYPT</data_protection_kind>
</topic_rule></topic_access_rules></domain_rule></domain_access_rules></dds>
XML
sign_policy "$OUT/governance.xml" "$OUT/governance.p7s"
permissions server server 0
for i in {1..6}; do permissions client "client$i" "$i"; done
if find "$OUT" -name '*.key' ! -perm 600 | read -r; then echo 'Unsafe private-key permissions' >&2; exit 1; fi
printf 'SECURITY_DIR=%s\n' "$OUT"
