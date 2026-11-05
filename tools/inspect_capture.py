#!/usr/bin/env python3
"""Count outer DDS security wrappers in a classic pcap without decrypting data.

CSD3116 team with declared AI assistance, 2026-11-04. This verifies packet
structure, not plaintext absence or GCM authenticity. No packet bytes/keys shown.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import struct


def packets(path):
    """Stream bounded classic-pcap records; reject truncated or unsupported files."""
    with path.open('rb') as stream:
        header = stream.read(24)
        orders = {b'\xd4\xc3\xb2\xa1': '<', b'\xa1\xb2\xc3\xd4': '>',
                  b'\x4d\x3c\xb2\xa1': '<', b'\xa1\xb2\x3c\x4d': '>'}
        if len(header) != 24 or header[:4] not in orders:
            raise ValueError('Expected classic pcap (convert pcapng with editcap first)')
        order = orders[header[:4]]
        _, major, minor, _, _, _, link = struct.unpack(order + 'IHHIIII', header)
        if (major, minor) != (2, 4) or link not in (0, 1, 101, 108, 113, 276):
            raise ValueError('Unsupported pcap version/link type')
        while record := stream.read(16):
            if len(record) != 16:
                raise ValueError('Incomplete pcap record header')
            _, _, captured, original = struct.unpack(order + 'IIII', record)
            if captured > 1024 * 1024 or captured > original:
                raise ValueError('Invalid or excessive packet size')
            data = stream.read(captured)
            if len(data) != captured:
                raise ValueError('Incomplete pcap packet')
            yield link, data, captured < original


def ipv4(link, data):
    """Remove supported link headers and return IPv4 bytes, never packet values."""
    if link == 1:
        if len(data) < 14:
            return b''
        offset, kind = 14, int.from_bytes(data[12:14], 'big')
        for _ in range(2):
            if kind not in (0x8100, 0x88a8) or len(data) < offset + 4:
                break
            kind = int.from_bytes(data[offset + 2:offset + 4], 'big')
            offset += 4
        return data[offset:] if kind == 0x0800 and len(data) > offset and data[offset] >> 4 == 4 else b''
    offset = {0: 4, 101: 0, 108: 4, 113: 16, 276: 20}[link]
    return data[offset:] if len(data) > offset and data[offset] >> 4 == 4 else b''


def inspect_rtps(data, counts):
    """Walk outer RTPS submessages using installed q_protocol.h IDs/endian flag."""
    if len(data) < 20 or data[:4] != b'RTPS':
        return
    counts['rtps_datagrams'] += 1
    offset = 20
    while offset < len(data):
        if len(data) - offset < 4:
            counts['malformed_rtps'] += 1
            return
        kind, flags = data[offset:offset + 2]
        size = int.from_bytes(data[offset + 2:offset + 4], 'little' if flags & 1 else 'big')
        end = offset + 4 + size if size else len(data)
        if end > len(data):
            counts['malformed_rtps'] += 1
            return
        body = data[offset + 4:end]
        if kind in (0x31, 0x33):
            counts['security_prefixes'] += 1
            # crypto_transform.c:set_crypto_prefix writes transformation kind BE.
            if len(body) >= 20 and int.from_bytes(body[:4], 'big') == 4:
                counts['aes256_gcm_prefixes'] += 1
        elif kind == 0x30:
            if len(body) < 4 or int.from_bytes(body[:4], 'big') > len(body) - 4:
                counts['malformed_rtps'] += 1
                return
            counts['encrypted_body_wrappers'] += 1
            counts['encrypted_body_bytes'] += int.from_bytes(body[:4], 'big')
        elif kind in (0x15, 0x16):
            counts['outer_data_or_fragment'] += 1
        offset = end


def main():
    """Summarize only; absence of wrappers fails instead of inventing proof."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('pcap', type=Path)
    args = parser.parse_args()
    counts = Counter()
    for link, data, truncated in packets(args.pcap):
        counts['packets'] += 1
        counts['truncated_packets'] += truncated
        ip = ipv4(link, data)
        if len(ip) < 20 or ip[9] != 17:
            continue
        length, header = int.from_bytes(ip[2:4], 'big'), (ip[0] & 15) * 4
        if header < 20 or length < header + 8 or length > len(ip):
            counts['incomplete_ipv4'] += 1
            continue
        counts['ipv4_udp_packets'] += 1
        if int.from_bytes(ip[6:8], 'big') & 0x3fff:
            counts['ipv4_fragment_packets_skipped'] += 1
            continue
        udp = ip[header:length]
        size = int.from_bytes(udp[4:6], 'big')
        if size < 8 or size > len(udp):
            counts['incomplete_udp'] += 1
            continue
        inspect_rtps(udp[8:size], counts)
    print(json.dumps(dict(sorted(counts.items())), indent=2))
    print('Outer wrappers only: public discovery/handshake data may remain visible. '
          'No decryption, authentication, topic identification or plaintext-absence proof.')
    print('Fragmented IPv4 needs separate offline reassembly; packet/time bounds may cut a datagram.')
    complete = not any(counts[key] for key in ('truncated_packets', 'incomplete_ipv4', 'incomplete_udp', 'malformed_rtps'))
    return 0 if complete and counts['aes256_gcm_prefixes'] and counts['encrypted_body_wrappers'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
