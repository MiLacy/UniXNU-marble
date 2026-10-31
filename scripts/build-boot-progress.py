#!/usr/bin/env python3
"""Build the board's on-screen boot progress archive for this source kernel.

BootProgress.c runs on the kernel side and provides the boot_progress_*
hooks plus a once-per-second framebuffer heartbeat. It is compiled with the
actual compile record of startup.o so the ABI matches the kernel being built.
"""
import hashlib
import json
import shutil
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/boot-progress'
OUT.mkdir(parents=True, exist_ok=True)
record_path = ROOT / 'build/xnu/ios-obj/DEVELOPMENT_ARM64_BCM2837/osfmk/DEVELOPMENT/startup.o.json'
record = json.loads(record_path.read_text().strip().rstrip(','))
source = ROOT / 'kits/marble/BootProgress.c'
obj = OUT / 'BootProgress.o'

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

font_dir = ROOT / 'drivers/DarwinBoot/DarwinBootPkg/Driver'
if not (font_dir / 'DiagFont.h').is_file():
    raise SystemExit(f'{font_dir}/DiagFont.h missing; the DarwinBoot driver carries the font')
command += ['-I', str(font_dir)]
subprocess.run(command, cwd=record['directory'], check=True, timeout=120)
(OUT / 'compile-command.json').write_text(json.dumps(command, indent=2) + '\n')

policy_record = json.loads((ROOT / 'build/xnu/ios-obj/DEVELOPMENT_ARM64_BCM2837/bsd/DEVELOPMENT/kern_exec.o.json').read_text().strip().rstrip(','))
policy_source = ROOT / 'kits/marble/BootLaunchPolicy.c'
policy_obj = OUT / 'BootLaunchPolicy.o'
policy_command = []
policy_args = policy_record['arguments']
j = 0
while j < len(policy_args):
    value = policy_args[j]
    if value == policy_record['file']:
        policy_command.append(str(policy_source))
    elif value in ('-o', '-MF', '-MT', '-MJ'):
        if value == '-o':
            policy_command += ['-o', str(policy_obj)]
        j += 1
    elif value in ('-MD', '-MP'):
        pass
    elif value == '-Xclang' and j + 1 < len(policy_args) and policy_args[j + 1].startswith('-W'):
        j += 1
    elif value.startswith('-W'):
        pass
    else:
        policy_command.append(value)
    j += 1
subprocess.run(policy_command, cwd=policy_record['directory'], check=True, timeout=120)

archive = OUT / 'libBootProgress.a'
archive.unlink(missing_ok=True)
archiver = ['llvm-ar'] if shutil.which('llvm-ar') else ['xcrun', 'ar']
subprocess.run([*archiver, 'rcs', str(archive), str(obj), str(policy_obj)], check=True, timeout=60)

(OUT / 'build-report.json').write_text(json.dumps({
    'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
    'launch_policy_sha256': hashlib.sha256(policy_source.read_bytes()).hexdigest(),
    'archive_sha256': hashlib.sha256(archive.read_bytes()).hexdigest(),
    'hardware_execution': False,
}, indent=2) + '\n')
print(f'PASS: boot progress archive {archive} ({archive.stat().st_size} bytes)')
