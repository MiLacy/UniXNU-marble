#!/usr/bin/env python3
"""Rebuild the two-module 21H461 trust bundle from the fetched IPSW members.

The payload accepts only the bundle hash pinned by the iPadOS profile. This
script extracts the version-2 payloads from the StaticTrustCache and
Cryptex1,SystemTrustCache IM4P files and writes the same two-module bundle
that scripts/build-original-system-trust.py builds from a full IPSW.

Run from the assembled tree after "unixnu ipsw fetch StaticTrustCache
Cryptex1,SystemTrustCache":
  python3 scripts/make-marble-trust.py
"""
from pathlib import Path
import hashlib
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = '7416ca4f8377146626bb284e2133c76bee6cb4e3ee0af5d648b186235a7323a7'


def der(data, at):
    tag, size = data[at], data[at + 1]
    at += 2
    if size & 128:
        count = size & 127
        size = int.from_bytes(data[at:at + count], 'big')
        at += count
    return tag, data[at:at + size], at + size


def module(path):
    data = path.read_bytes()
    tag, sequence, end = der(data, 0)
    values, cursor = [], 0
    while cursor < len(sequence):
        kind, value, cursor = der(sequence, cursor)
        values.append(value)
    if tag != 0x30 or end != len(data) or len(values) < 4:
        raise SystemExit(f'{path}: not the expected IM4P wrapper')
    payload = values[-1]
    count = struct.unpack_from('<I', payload, 20)[0]
    if struct.unpack_from('<I', payload)[0] != 2 or len(payload) != 24 + 24 * count:
        raise SystemExit(f'{path}: not a complete version-2 trust module')
    return payload


def cache():
    for parent in ROOT.parents:
        candidate = parent / 'ipsw/21H461'
        if candidate.is_dir():
            return candidate
    raise SystemExit('IPSW cache not found; run "unixnu ipsw fetch StaticTrustCache Cryptex1,SystemTrustCache"')


ipsw = cache()
static = module(ipsw / '044-76040-038.dmg.trustcache')
system = module(ipsw / '044-76034-038.dmg.trustcache')
if hashlib.sha256(static).hexdigest() != 'da88460e6e513438d5307e5c526f76497a74754476a24da29f239f0c90120fc2':
    raise SystemExit('Static trust module differs from the 21H461 input')
if hashlib.sha256(system).hexdigest() != 'd121ba89167e03aafd00fa687c30cd8ebc4ccafd10c1408adb2ba3de12403651':
    raise SystemExit('System trust module differs from the 21H461 input')
bundle = struct.pack('<3I', 2, 12, 12 + len(static)) + static + system
digest = hashlib.sha256(bundle).hexdigest()
if digest != EXPECTED:
    raise SystemExit(f'bundle hash {digest} is not the profile-pinned {EXPECTED}')
out = ROOT / 'build/marble-trust.bundle'
out.parent.mkdir(parents=True, exist_ok=True)
out.write_bytes(bundle)
print(f'{out} {digest}')
