#!/usr/bin/env python3
"""Build the CPU display provider archive the kernel links under
CONFIG_XNU_NATIVE_MOBILE_DISPLAY (build/gop-framebuffer/libGopFramebuffer.late.a).

The archive carries the self-contained IOMobileFramebuffer/IOCoreSurfaceRoot
implementation from the marble CPU-display reference; the original
IOMobileGraphicsFamily and IOSurface kexts are removed from the prelink so the
class names are unique.
"""
import hashlib
import json
import shutil
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/gop-framebuffer'
OUT.mkdir(parents=True, exist_ok=True)

SOURCE = ROOT / 'kits/marble/DisplayProbe/MarbleDisplay.cpp'
SOURCES = [SOURCE]
BINDINGS = ROOT / 'kits/marble/gop-framebuffer-bindings.S'

BINDINGS.parent.mkdir(parents=True, exist_ok=True)
BINDINGS.write_text('# SPDX-License-Identifier: MIT\n'
    '.globl __ZN19IOMobileFramebuffer10gMetaClassE\n.set __ZN19IOMobileFramebuffer10gMetaClassE, 0xfffffff006b313d8\n'
    '.globl __ZN19IOMobileFramebufferC2EPK11OSMetaClass\n.set __ZN19IOMobileFramebufferC2EPK11OSMetaClass, 0xfffffff00577b318\n'
    '.globl __ZN19IOMobileFramebufferD2Ev\n.set __ZN19IOMobileFramebufferD2Ev, 0xfffffff00577b364\n')

record = json.loads((ROOT / 'build/xnu/ios-obj/DEVELOPMENT_ARM64_BCM2837/iokit/DEVELOPMENT/IOPlatformExpert.cpo.json').read_text().strip().rstrip(','))
object_paths = []
for source in SOURCES:
    obj = OUT / (source.stem + '.o')
    command = []
    args = record['arguments']
    index = 0
    while index < len(args):
        value = args[index]
        if value == record['file']:
            command.append(str(source))
        elif value in ('-o', '-MF', '-MT', '-MJ'):
            if value == '-o':
                command += ['-o', str(obj)]
            index += 1
        elif value in ('-MD', '-MP'):
            pass
        elif value == '-Xclang' and index + 1 < len(args) and args[index + 1].startswith('-W'):
            index += 1
        elif value.startswith('-W'):
            pass
        else:
            command.append(value)
        index += 1
    command += ['-Wno-error', '-I', str(SOURCES[0].parent)]
    subprocess.run(command, cwd=record['directory'], check=True, timeout=180)
    object_paths.append(obj)

compiler = shutil.which('clang') or '/opt/unixnu-linux/llvm/bin/clang'
bindings_obj = OUT / 'gop-framebuffer-bindings.o'
subprocess.run([compiler, '-target', 'arm64-apple-ios17.0', '-mkernel', '-c', str(BINDINGS),
                '-o', str(bindings_obj)], check=True, timeout=120)

archive = OUT / 'libGopFramebuffer.late.a'
archive.unlink(missing_ok=True)
archiver = ['llvm-ar'] if shutil.which('llvm-ar') else ['xcrun', 'ar']
subprocess.run([*archiver, 'rcs', str(archive), *[str(p) for p in object_paths], str(bindings_obj)], check=True, timeout=60)

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

(OUT / 'build-report.json').write_text(json.dumps({
    'source_sha256': {str(s.relative_to(ROOT)): sha(s) for s in SOURCES},
    'bindings_sha256': sha(BINDINGS),
    'archive_sha256': sha(archive),
    'hardware_execution': False,
}, indent=2) + '\n')
print(f'PASS: display provider archive {archive} ({archive.stat().st_size} bytes)')
