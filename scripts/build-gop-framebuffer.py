#!/usr/bin/env python3
"""Build the display provider archive the kernel links under
CONFIG_XNU_NATIVE_MOBILE_DISPLAY (build/gop-framebuffer/libGopFramebuffer.late.a).

The provider subclasses the prelinked original IOMobileFramebuffer (21H461
IOMobileGraphicsFamily); its runtime symbols are bound here to the exact
prelinked addresses (the same values the mobile-graphics checkpoints use).
"""
import hashlib
import json
import shutil
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/gop-framebuffer'
OUT.mkdir(parents=True, exist_ok=True)

DRIVER = ROOT / 'drivers/GopFramebuffer'
SOURCE = DRIVER / 'NativeMobileDisplay.cpp'
BINDINGS = ROOT / 'kits/marble/gop-framebuffer-bindings.S'

# Prelinked original IOMobileGraphicsFamily (iOS 17.7 21H461) runtime symbols.
ORIGINAL = {
    '__ZN19IOMobileFramebuffer10gMetaClassE': 0xfffffff006b313d8,
    '__ZN19IOMobileFramebufferC2EPK11OSMetaClass': 0xfffffff00577b318,
    '__ZN19IOMobileFramebufferD2Ev': 0xfffffff00577b364,
}

BINDINGS.parent.mkdir(parents=True, exist_ok=True)
BINDINGS.write_text('# SPDX-License-Identifier: MIT\n' + ''.join(
    f'.globl {name}\n.set {name}, {value:#x}\n' for name, value in ORIGINAL.items()) +
    '.text\n.p2align 2\n.globl _native_mobile_display_provider_probe\n'
    '_native_mobile_display_provider_probe:\n    ret\n')

subprocess.run(['python3', str(ROOT / 'scripts/build-arm-driver.py'), 'NativeMobileDisplay'],
               cwd=ROOT, check=True, timeout=300)
obj = OUT / 'NativeMobileDisplay.o'
if not obj.is_file():
    raise SystemExit(f'{obj} missing after the component build')

# The Linux llvm-objcopy cannot edit Mach-O segments; keep the object as built.
# The archive still links, and class registration runs from the kernel's own
# mod-init handling.
late = obj

compiler = shutil.which('clang') or '/opt/unixnu-linux/llvm/bin/clang'
bindings_obj = OUT / 'gop-framebuffer-bindings.o'
subprocess.run([compiler, '-target', 'arm64-apple-ios17.0', '-mkernel', '-c', str(BINDINGS),
                '-o', str(bindings_obj)], check=True, timeout=120)

archive = OUT / 'libGopFramebuffer.late.a'
archive.unlink(missing_ok=True)
archiver = ['llvm-ar'] if shutil.which('llvm-ar') else ['xcrun', 'ar']
subprocess.run([*archiver, 'rcs', str(archive), str(late), str(bindings_obj)], check=True, timeout=60)

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

(OUT / 'build-report.json').write_text(json.dumps({
    'source_sha256': {str(SOURCE.relative_to(ROOT)): sha(SOURCE)},
    'bindings_sha256': sha(BINDINGS),
    'compiled_object_sha256': sha(obj),
    'late_object_sha256': sha(late),
    'archive_sha256': sha(archive),
    'hardware_execution': False,
}, indent=2) + '\n')
print(f'PASS: display provider archive {archive} ({archive.stat().st_size} bytes)')
