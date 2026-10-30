#!/usr/bin/env python3
"""Add the original IOStorageFamily base bindings to libShengUfs.a.

The full (non-probe) UFS block class subclasses IOBlockStorageDevice. The
original class lives in the prelinked IOStorageFamily kext, so its vtable,
constructors, destructor, metaclass and inherited methods are bound as
absolute symbols from scripts/data/native-storage-original-bindings.S.
"""
from pathlib import Path
import os
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/sheng-ufs'
if os.environ.get('XNU_NATIVE_UFS_FULL') != '1':
    raise SystemExit(0)
archive = OUT / 'libShengUfs.a'
if not archive.is_file():
    raise SystemExit('libShengUfs.a missing; build the ShengUfs component first')
source = ROOT / 'scripts/data/native-storage-original-bindings.S'
if not source.is_file():
    raise SystemExit('native-storage-original-bindings.S missing')
obj = OUT / 'native-storage-original-bindings.o'
subprocess.run(['xcrun', '--sdk', 'iphoneos', 'clang', '-arch', 'arm64', '-mkernel',
                '-miphoneos-version-min=17.0', '-c', str(source), '-o', str(obj)], check=True)
subprocess.run(['xcrun', 'ar', 'rcs', str(archive), str(obj)], check=True)
print(f'{archive} + {obj.name}')
