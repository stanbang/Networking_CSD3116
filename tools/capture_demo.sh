#!/usr/bin/env bash
# Purpose: bounded full-packet secure capture with explicit baseline mode. Team + AI, 2026-11-05.
# Human invokes with sudo if needed; this script never invokes sudo itself.
# Output stays in the invoking user's Linux home, where mode 0600 can be verified.
# IPv4 RTPS magic selects DDS packets even when Cyclone chooses ephemeral UDP ports.
# Source: installed dds/ddsi/q_protocol.h Header_t and NN_PROTOCOLID_AS_UINT32.
set -euo pipefail
umask 077
b_secure=false
b_baseline=false
if [[ ${1:-} == --secure ]]; then b_secure=true; shift;
elif [[ ${1:-} == --baseline ]]; then b_baseline=true; shift; fi
usage() {
    echo 'Usage: bash tools/capture_demo.sh [--secure|--baseline] [seconds 1..120] [basename.pcap] [interface=lo]'
    echo 'Default: full unfragmented IPv4 RTPS. --secure: full IPv4 UDP including fragments.'
    echo '--baseline: truncated P0 reference capture only.'
    echo 'Capture only an authorised interface/session; unrelated UDP may be included.'
}
if [[ ${1:-} == --help ]]; then usage; exit 0; fi
SECONDS_TO_CAPTURE="${1:-30}"
str_default=secure-demo.pcap
if [[ $b_baseline == true ]]; then str_default=p0-demo.pcap; fi
BASENAME="${2:-$str_default}"
INTERFACE="${3:-lo}"
if (( $# > 3 )) || [[ ! "$SECONDS_TO_CAPTURE" =~ ^[0-9]{1,3}$ ]] ||
   (( 10#$SECONDS_TO_CAPTURE < 1 || 10#$SECONDS_TO_CAPTURE > 120 )); then
    usage >&2
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
if [[ $b_secure == false && -n "${CYCLONEDDS_URI:-}" ]]; then
    echo 'Unset CYCLONEDDS_URI for the default capture guide (secure config: domain42/lo).' >&2
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
int_snaplen=65535
int_packets=4000
str_filter='ip and udp and udp[8:4] = 0x52545053'
if [[ $b_secure == true ]]; then
    int_snaplen=0
    int_packets=2000
    # A UDP-port/RTPS-header filter would discard later IPv4 fragments. Preserve
    # the complete IPv4 UDP stream for offline reassembly and DDS inspection.
    str_filter='ip proto 17'
    echo "Secure-game evidence capture: $INTERFACE, ${SECONDS_TO_CAPTURE}s, at most $int_packets packets."
    echo 'Full snapshot length (-s 0); includes IPv4 UDP fragments, no IPv6.'
    echo 'All IPv4 UDP on this interface may be recorded, including unrelated traffic.'
    echo 'Run the secure game in another terminal; inspect the actual pcap before claiming encryption.'
elif [[ $b_baseline == true ]]; then
    int_snaplen=256
    int_packets=10000
    echo "P0 PLAINTEXT capture: $INTERFACE, IPv4 RTPS, ${SECONDS_TO_CAPTURE}s."
    echo 'Bound: 10000 packets, 256 captured bytes per packet (P0-only, truncated).'
    echo 'Other IPv4 DDS traffic may be recorded; no IPv6 or later IP fragments.'
    echo 'Use play.sh --baseline for P0; loopback may be empty if the baseline uses eth0.'
else
    echo "DDS capture: $INTERFACE, IPv4 RTPS, ${SECONDS_TO_CAPTURE}s."
    echo 'Bound: 4000 packets, 65535 bytes each; full unfragmented IPv4 UDP datagrams.'
    echo 'RTPS filter omits later IP fragments; no IPv6/reassembly. --secure retains IPv4 UDP fragments.'
    echo 'Start capture first, then the secure game. Inspect actual RTPS security structure offline.'
fi
printf 'Filter: %s\n' "$str_filter"
set +e
timeout --signal=INT --kill-after=3s "${SECONDS_TO_CAPTURE}s" \
    tcpdump -n -i "$INTERFACE" -s "$int_snaplen" -c "$int_packets" -U -Z "$CAPTURE_USER" \
    -w "$TEMP" "$str_filter"
RESULT=$?
set -e
if (( RESULT != 0 && RESULT != 124 )); then
    echo "Capture failed (exit $RESULT). Run this script with the same arguments in a human sudo terminal if needed." >&2
    exit "$RESULT"
fi
if (( $(stat -c %s -- "$TEMP") <= 24 )); then
    echo 'No packets captured; verify the game is running and the selected interface. No evidence produced.' >&2
    exit 1
fi
# tcpdump's stderr includes its packet counts; a complete packet must also exist.
tcpdump -n -Z "$CAPTURE_USER" -r "$TEMP" -c 1 2>/dev/null
# A hard link publishes the completed file atomically and refuses overwrites.
ln -- "$TEMP" "$DEST/$BASENAME"
echo "Saved $DEST/$BASENAME (private mode; do not commit or submit captures)."
printf 'Unprivileged inspection: tcpdump -n -r %q\n' "$DEST/$BASENAME"
