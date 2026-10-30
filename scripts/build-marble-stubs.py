#!/usr/bin/env python3
"""Build the weak first-console stubs for the marble kernel link."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/marble-stubs'
OUT.mkdir(parents=True, exist_ok=True)
sources = sorted(ROOT.glob('kits/*/MarbleStubs.c'))
if len(sources) != 1:
    raise SystemExit('Expected one device package with MarbleStubs.c')
source = sources[0]
obj = OUT / 'MarbleStubs.o'
subprocess.run(['xcrun', '--sdk', 'iphoneos', 'clang', '-arch', 'arm64', '-mkernel',
                '-miphoneos-version-min=17.0', '-Os', '-Wall', '-Werror',
                '-c', str(source), '-o', str(obj)], check=True)
archive = OUT / 'libMarbleStubs.a'
archive.unlink(missing_ok=True)
subprocess.run(['xcrun', 'ar', 'rcs', str(archive), str(obj)], check=True)
print(archive)
