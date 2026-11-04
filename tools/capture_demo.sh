#!/usr/bin/env bash
# Purpose: bounded, local secure-game DDS capture. Author: CSD3116 team + AI; updated 2026-11-05.
# Human invokes with sudo if needed; this script never invokes sudo itself.
# Output stays in the invoking user's Linux home, where mode 0600 can be verified.
# IPv4 RTPS magic selects DDS packets even when Cyclone chooses ephemeral UDP ports.
# Source: installed dds/ddsi/q_protocol.h Header_t and NN_PROTOCOLID_AS_UINT32.
set -euo pipefail
umask 077
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
SECONDS_TO_CAPTURE="${1:-30}"
BASENAME="${2:-secure-demo.pcap}"
INTERFACE="${3:-lo}"
if (( $# > 3 )) || [[ ! "$SECONDS_TO_CAPTURE" =~ ^[0-9]{1,3}$ ]] ||
   (( 10#$SECONDS_TO_CAPTURE < 1 || 10#$SECONDS_TO_CAPTURE > 120 )); then
    echo 'Usage: bash tools/capture_demo.sh [seconds 1..120] [basename.pcap] [interface=lo]' >&2
    echo 'Selects IPv4 RTPS on any UDP port; default lo, or select another active interface.' >&2
    exit 1
fi
if [[ ! "$INTERFACE" =~ ^[a-zA-Z0-9][a-zA-Z0-9_.-]*$ ||
      ! -d "/sys/class/net/$INTERFACE" ]]; then
    echo 'Select one existing interface, for example lo or eth0; any is disallowed.' >&2
    exit 1
fi
if [[ ! "$BASENAME" =~ ^[a-zA-Z0-9][a-zA-Z0-9_.-]*\.pcap$ ]]; then
    echo 'Output must be a plain .pcap basename; no directory components.' >&2
    exit 1
fi
for TOOL in tcpdump timeout mktemp; do
    command -v "$TOOL" >/dev/null || { echo "Missing $TOOL" >&2; exit 1; }
done
if [[ -n "${CYCLONEDDS_URI:-}" ]]; then
    echo 'Unset CYCLONEDDS_URI; this guide verifies config/cyclonedds.xml (domain42, loopback).'  >&2
    exit 1
fi
CAPTURE_USER="$(id -un)"
CAPTURE_UID="$(id -u)"
CAPTURE_GID="$(id -g)"
if (( EUID == 0 )); then
    if [[ ! "${SUDO_UID:-}" =~ ^[0-9]+$ || ! "${SUDO_GID:-}" =~ ^[0-9]+$ ||
          -z "${SUDO_USER:-}" ]] ||
       [[ "$(id -u -- "$SUDO_USER")" != "$SUDO_UID" ||
          "$(id -g -- "$SUDO_USER")" != "$SUDO_GID" ]]; then
        echo 'Invalid sudo ownership metadata; refusing capture.' >&2
        exit 1
    fi
    CAPTURE_USER="$SUDO_USER"
    CAPTURE_UID="$SUDO_UID"
    CAPTURE_GID="$SUDO_GID"
fi
CAPTURE_HOME="$(getent passwd "$CAPTURE_UID" | cut -d: -f6)"
if [[ -z "$CAPTURE_HOME" || "$CAPTURE_HOME" != /* ||
      "$(realpath -- "$CAPTURE_HOME")" != "$CAPTURE_HOME" ||
      "$(stat -c %u -- "$CAPTURE_HOME")" != "$CAPTURE_UID" ]]; then
    echo 'Invoking user needs an owned, nonsymlinked Linux home directory.' >&2
    exit 1
fi
DEST="$CAPTURE_HOME/.local/state/dds-asteroids/captures"
for DIR in "$CAPTURE_HOME/.local" "$CAPTURE_HOME/.local/state" \
           "$CAPTURE_HOME/.local/state/dds-asteroids" "$DEST"; do
    if [[ "$(realpath -m -- "$DIR")" != "$DIR" ]]; then
        echo 'Refusing symlinked capture path.' >&2
        exit 1
    fi
    if [[ ! -d "$DIR" ]]; then
        mkdir -m 700 -- "$DIR"
        if (( EUID == 0 )); then chown -- "$CAPTURE_UID:$CAPTURE_GID" "$DIR"; fi
    fi
    if [[ "$(stat -c %u -- "$DIR")" != "$CAPTURE_UID" ]]; then
        echo 'Refusing unowned capture path.' >&2
        exit 1
    fi
done
chmod 700 -- "$DEST"
if [[ "$(stat -c %a -- "$DEST")" != 700 ||
      -e "$DEST/$BASENAME" || -L "$DEST/$BASENAME" ]]; then
    echo 'Refusing symlinked/unowned capture directory or existing output.' >&2
    exit 1
fi
TEMP="$(mktemp "$DEST/.dds-capture.XXXXXXXX")"
trap 'rm -f -- "$TEMP"' EXIT
if (( EUID == 0 )); then chown -- "$CAPTURE_UID:$CAPTURE_GID" "$TEMP"; fi
chmod 600 -- "$TEMP"
if [[ "$(stat -c %a -- "$TEMP")" != 600 ||
      "$(stat -c %u -- "$TEMP")" != "$CAPTURE_UID" ]]; then
    echo 'This filesystem cannot provide private user-owned capture files.' >&2
    exit 1
fi
echo "DDS capture: $INTERFACE, IPv4 RTPS on any UDP port, ${SECONDS_TO_CAPTURE}s."
echo 'Bound: 4000 packets, 65535 captured bytes per packet; full IPv4 UDP datagrams when unfragmented.'
echo 'Filter: ip and udp and udp[8:4] = 0x52545053 (RTPS header; no authentication).'
echo 'Other IPv4 DDS/RTPS traffic on this interface can be captured; no domain isolation.'
echo 'No IPv6 or IP-fragment reassembly. Analysis must check truncation and RTPS security structure.'
echo 'Current secure config uses domain42 on lo. Baseline/other configurations require their actual interface.'
echo 'Start capture first, then launch the secure game in another terminal. Capture alone does not prove encryption.'
set +e
timeout --signal=INT --kill-after=3s "${SECONDS_TO_CAPTURE}s" \
    tcpdump -n -i "$INTERFACE" -s 65535 -c 4000 -U -Z "$CAPTURE_USER" \
    -w "$TEMP" 'ip and udp and udp[8:4] = 0x52545053'
RESULT=$?
set -e
if (( RESULT != 0 && RESULT != 124 )); then
    echo "Capture failed (exit $RESULT). Human may run: sudo bash tools/capture_demo.sh" >&2
    exit "$RESULT"
fi
if (( $(stat -c %s -- "$TEMP") <= 24 )); then
    echo 'No packets captured; check game activity/interface. No evidence produced.' >&2
    exit 1
fi
# tcpdump's stderr includes its packet counts; a complete packet must also exist.
tcpdump -n -Z "$CAPTURE_USER" -r "$TEMP" -c 1 2>/dev/null
# A hard link publishes the completed file atomically and refuses overwrites.
ln -- "$TEMP" "$DEST/$BASENAME"
echo "Saved $DEST/$BASENAME (private mode; do not commit or submit captures)."
printf 'Unprivileged inspection: tcpdump -n -r %q\n' "$DEST/$BASENAME"
