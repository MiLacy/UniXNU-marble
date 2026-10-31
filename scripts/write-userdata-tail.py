#!/usr/bin/env python3
"""Write the marble root image into the userdata tail without touching Android.

The GPT userdata entry starts at absolute LBA 3107328. The root window starts at
absolute LBA 59589627 and is 2290688 4 KiB blocks long, ending at the last LBA of
userdata. Because `dd` writes the *partition* device, its seek is relative to the
partition start; Android's ext4 data at the start is left untouched.

The device must be in TWRP/recovery (or rooted Android with userdata unmounted).
Dry run is the default; pass --execute after checking the printed command.
"""
import argparse
import hashlib
import subprocess
import sys
from pathlib import Path

FIRST_LBA = 3107328
ROOT_LBA = 59589627
ROOT_BLOCKS = 2290688
BLOCK = 4096
SEEK = ROOT_LBA - FIRST_LBA
EXPECTED_BYTES = ROOT_BLOCKS * BLOCK


def run(command, **kwargs):
    print('+', ' '.join(command), flush=True)
    return subprocess.run(command, text=True, **kwargs)


def adb(args, **kwargs):
    return run(['adb', 'shell', *args], **kwargs)


def device_root(command, **kwargs):
    """Run a command as root: direct in TWRP, through su on rooted Android."""
    uid = run(['adb', 'shell', 'id', '-u'], capture_output=True)
    if uid.stdout.strip() == '0':
        return adb([command], **kwargs)
    return adb([f'su -c "{command}"'], **kwargs)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('image', type=Path)
    parser.add_argument('--block-device', default='/dev/block/by-name/userdata')
    parser.add_argument('--remote-image', default='/tmp/marble-root.img')
    parser.add_argument('--no-push', action='store_true',
                        help='use an image already present at --remote-image')
    parser.add_argument('--execute', action='store_true',
                        help='perform the write; without it only print the commands')
    args = parser.parse_args()

    if not args.image.is_file():
        parser.error(f'image not found: {args.image}')
    size = args.image.stat().st_size
    if size != EXPECTED_BYTES:
        parser.error(f'image is {size} bytes, expected exactly {EXPECTED_BYTES} '
                     f'({ROOT_BLOCKS} x {BLOCK})')
    digest = hashlib.sha256()
    with args.image.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(chunk)
    digest = digest.hexdigest()
    print(f'image: {args.image}')
    print(f'sha256: {digest}')
    print(f'userdata first LBA: {FIRST_LBA}')
    print(f'root window: LBA {ROOT_LBA}..{ROOT_LBA + ROOT_BLOCKS - 1} '
          f'({ROOT_BLOCKS} blocks, {size / 2**30:.3f} GiB)')
    print(f'partition-relative dd seek: {SEEK} blocks ({SEEK * BLOCK} bytes)')
    print(f'Android data at partition offset 0 is not in the written range.')
    print()
    print('Run this only from TWRP/recovery with userdata unmounted:')
    print(f'  adb push {args.image} {args.remote_image}')
    print(f'  adb shell "dd if={args.remote_image} of={args.block_device} '
          f'bs={BLOCK} seek={SEEK} count={ROOT_BLOCKS} conv=notrunc,fsync"')
    print('  adb shell "sync"')
    print('  (the helper adds `su -c` only when adb is not already uid 0)')
    print()

    if not args.execute:
        print('dry run: nothing written; pass --execute to perform it')
        return 0

    devices = run(['adb', 'devices'], capture_output=True)
    if devices.returncode or '\tdevice' not in devices.stdout:
        print('adb device is not ready; reboot to TWRP/recovery and retry', file=sys.stderr)
        return 1
    if args.no_push:
        remote_hash = adb([f'sha256sum {args.remote_image}'], capture_output=True)
        if remote_hash.returncode or not remote_hash.stdout.startswith(digest):
            print(f'{args.remote_image} is missing or does not match the host SHA-256',
                  file=sys.stderr)
            return 1
        print(f'remote image hash matches {digest}')
    elif run(['adb', 'push', str(args.image), args.remote_image]).returncode:
        return 1

    # Preserve and verify the first partition block (Android ext4 or its metadata).
    before = '/tmp/marble-userdata-head.before'
    after = '/tmp/marble-userdata-head.after'
    if device_root(f'dd if={args.block_device} of={before} bs={BLOCK} count=1').returncode:
        return 1
    if device_root(f'dd if={args.remote_image} of={args.block_device} bs={BLOCK} '
                   f'seek={SEEK} count={ROOT_BLOCKS} conv=notrunc,fsync').returncode:
        return 1
    if device_root('sync').returncode:
        return 1
    if device_root(f'dd if={args.block_device} of={after} bs={BLOCK} count=1').returncode:
        return 1
    compared = device_root(f'cmp -s {before} {after} && echo UNCHANGED || echo CHANGED',
                           capture_output=True)
    print(compared.stdout.strip())
    if 'UNCHANGED' not in compared.stdout:
        print('the first userdata block changed; stop and inspect before booting', file=sys.stderr)
        return 1
    manifest = adb([f'dd if={args.block_device} bs={BLOCK} skip={SEEK} count=1 2>/dev/null | '
                    f'od -An -tx1 -N8'], capture_output=True)
    magic = manifest.stdout.split()
    if ' '.join(magic) != '53 48 4e 47 52 4f 4f 54':
        print(f'root window does not start with SHNGROOT: {manifest.stdout!r}', file=sys.stderr)
        return 1
    print('root window starts with SHNGROOT; Android first block unchanged')
    print('tail write complete; boot the root image next')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
