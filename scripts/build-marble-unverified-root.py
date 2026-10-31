#!/usr/bin/env python3
"""Build the development-only unverified SystemOS volume lookup archive."""
import hashlib
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/marble-unverified-root'
OUT.mkdir(parents=True, exist_ok=True)
SOURCE = ROOT / 'kits/marble/MarbleUnverifiedRoot.cpp'
if not SOURCE.is_file():
    raise SystemExit(f'{SOURCE} is missing')
record = json.loads((ROOT / 'build/xnu/ios-obj/DEVELOPMENT_ARM64_BCM2837/iokit/DEVELOPMENT/IOPlatformExpert.cpo.json').read_text().strip().rstrip(','))
obj = OUT / 'MarbleUnverifiedRoot.o'
command = []
args = record['arguments']
index = 0
while index < len(args):
    value = args[index]
    if value == record['file']:
        command.append(str(SOURCE))
    elif value == '-o':
        command.extend(['-o', str(obj)])
        index += 1
    elif value in ('-MF', '-MJ'):
        index += 1
    elif value not in ('-MD', '-MP'):
        command.append(value)
    index += 1
command += ['-I', str(SOURCE.parent), '-Wno-unused-parameter']
result = subprocess.run(command, cwd=record['directory'], capture_output=True, text=True)
(OUT / 'compile.log').write_text(result.stdout + result.stderr)
if result.returncode:
    raise SystemExit((result.stdout + result.stderr)[-3500:])
archive = OUT / 'libMarbleUnverifiedRoot.a'
archive.unlink(missing_ok=True)
subprocess.run(['xcrun', 'ar', 'rcs', archive, obj], check=True)
(OUT / 'build-report.json').write_text(json.dumps({
    'source_sha256': hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
    'archive_sha256': hashlib.sha256(archive.read_bytes()).hexdigest(),
    'verified_root': False,
    'hardware_tested': False,
}, indent=2) + '\n')
print(f'PASS: unverified root lookup {archive} ({archive.stat().st_size} bytes)')
