#!/usr/bin/env python3
"""Add the original IOBlockStorageDevice ABI bindings to a storage archive.

Used by the verified owner-root build for NativeVirtioBlock and NativeRamBlock.
The original storage class lives in the prelinked IOStorageFamily kext; the
absolute bindings are the same ones used by the UFS full build.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'scripts/data/native-storage-original-bindings.S'

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--component', required=True)
parser.add_argument('--archive', required=True)
parser.add_argument('--directory', required=True)
parser.add_argument('--subset', action='store_true')
args = parser.parse_args()

directory = ROOT / args.directory
archive = directory / args.archive
if not archive.is_file():
    raise SystemExit(f'{archive} missing; build the component first')
if not SOURCE.is_file():
    raise SystemExit(f'{SOURCE} missing')

object_path = directory / 'native-storage-original-bindings.o'
subprocess.run(['xcrun', '--sdk', 'iphoneos', 'clang', '-arch', 'arm64', '-mkernel',
                '-miphoneos-version-min=17.0', '-c', str(SOURCE), '-o', str(object_path)], check=True)
subprocess.run(['xcrun', 'ar', 'rcs', str(archive), str(object_path)], check=True)

original = set(re.findall(r'^\.set\s+(\S+)\s*,', SOURCE.read_text(), re.M))
component_object = directory / (args.component + '.o')
if not component_object.is_file():
    component_object = next(iter(directory.glob('*.o')), None)
imported = set()
if component_object:
    nm = subprocess.run(['llvm-nm', '-u', str(component_object)], capture_output=True, text=True).stdout
    imported = {line.split()[-1] for line in nm.splitlines() if line.strip()}

bound = sorted(original & imported)
report = dict(component=args.component, archive=args.archive, directory=args.directory,
              original_symbols=sorted(original), imported_storage_symbols=sorted(imported & original),
              unbound_imports=sorted(imported - original), bound=bound,
              subset=bool(args.subset), object=component_object.name if component_object else None,
              compiled_object_sha256=hashlib.sha256(component_object.read_bytes()).hexdigest() if component_object else None,
              archive_sha256=hashlib.sha256(archive.read_bytes()).hexdigest())
if imported and not args.subset and (imported - original):
    raise SystemExit(f'{args.component} imports symbols without original bindings')
if args.subset and not bound:
    raise SystemExit(f'{args.component} imports none of the original storage symbols')
(directory / 'bindings.json').write_text(json.dumps(report, indent=2) + '\n')
print(f'{archive} + {object_path.name}; bound {len(bound)} original symbols')
