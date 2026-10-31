#!/usr/bin/env python3
"""Sign and fully verify an OS or SystemOS virtual volume on macOS.

This is the component-parameterized form of UniXNU's
scripts/build-owner-os-policy.py.  It requires the volume to be attached
read-only with hdiutil and verifies the same things the original script did:
the attached Disk Image is read-only, the volume UUID and total size match,
the trusted owner key signs the manifest, and every 4096-byte block passes the
real C/RSA verifier.

Run on macOS after:
  kits/VerifiedRoot/tests/check.py --output build/verified-root-host \
      --openssl-prefix "$(brew --prefix openssl@3)"
  hdiutil attach -readonly -nobrowse -mountpoint <mount> <dmg>
"""
import argparse
import ctypes
import hashlib
import importlib.util
import json
from pathlib import Path
import plistlib
import stat
import subprocess
import uuid

OS_UUID = '04769D26-2CA0-4825-B443-E2E5F88300F2'
OS_IMAGE_SHA256 = 'e5634d04a8ac35f024845f583394e0a35deb26a04cb0c00286a8674e25a9a4f2'
OS_BYTES = 6077580800
OS_BLOCKS = 1483785
SYSTEMOS_UUID = '9071148B-B025-402D-AD7A-48464D8B5FFF'
SYSTEMOS_IMAGE_SHA256 = 'b4908cde41d23e6bc05e6af51a2c9eeb00cf0f863a6a18b1600c96a3fd70414f'
SYSTEMOS_BYTES = 3303014400
SYSTEMOS_BLOCKS = 806400


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--unixnu', type=Path, required=True, help='UniXNU checkout')
    parser.add_argument('--component', choices=('os', 'systemos'), required=True)
    parser.add_argument('--source', type=Path, required=True, help='attached DMG file')
    parser.add_argument('--mountpoint', type=Path, required=True, help='hdiutil -mountpoint')
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--volume-uuid')
    parser.add_argument('--image-sha256')
    parser.add_argument('--image-bytes', type=int)
    parser.add_argument('--blocks', type=int)
    parser.add_argument('--source-sha256')
    parser.add_argument('--source-format', choices=('UDIF', 'UDRW'), required=True)
    return parser.parse_args()


def main():
    args = parse_args()
    root = args.unixnu.resolve()
    out = args.out.resolve()
    source = args.source.resolve()
    mountpoint = args.mountpoint.resolve()
    expected_uuid = uuid.UUID(args.volume_uuid or (OS_UUID if args.component == 'os' else SYSTEMOS_UUID))
    expected_sha = args.image_sha256 or (OS_IMAGE_SHA256 if args.component == 'os' else SYSTEMOS_IMAGE_SHA256)
    expected_bytes = args.image_bytes or (OS_BYTES if args.component == 'os' else SYSTEMOS_BYTES)
    expected_blocks = args.blocks or (OS_BLOCKS if args.component == 'os' else SYSTEMOS_BLOCKS)
    if not source.is_file():
        raise SystemExit(f'missing source DMG: {source}')
    out.mkdir(parents=True, exist_ok=True)

    source_digest = hashlib.sha256(source.read_bytes()).hexdigest()
    if args.source_sha256 and source_digest != args.source_sha256:
        raise SystemExit(f'{source}: source digest {source_digest} != {args.source_sha256}')
    if args.source_format == 'UDIF':
        with source.open('rb') as stream:
            stream.seek(-512, 2)
            if stream.read(4) != b'koly':
                raise SystemExit(f'{source}: expected a UDIF trailer')
    else:
        with source.open('rb') as stream:
            stream.seek(-512, 2)
            if stream.read(4) == b'koly':
                raise SystemExit(f'{source}: expected a raw UDRW, found a UDIF trailer')

    images = plistlib.loads(subprocess.check_output(['hdiutil', 'info', '-plist']))['images']
    found = [i for i in images if Path(i['image-path']).resolve() == source and i.get('writeable') is False]
    if len(found) != 1:
        raise SystemExit('Require one existing exact read-only image attachment')
    entities = found[0]['system-entities']
    whole = [e['dev-entry'] for e in entities if e.get('content-hint') == 'GUID_partition_scheme']
    if len(whole) != 1:
        raise SystemExit('Require one actual whole virtual media mapping')
    info = plistlib.loads(subprocess.check_output(['diskutil', 'info', '-plist', whole[0]]))
    volume_info = plistlib.loads(subprocess.check_output(['diskutil', 'info', '-plist', str(mountpoint)]))
    if (info.get('BusProtocol') != 'Disk Image' or info.get('Writable') is not False or
            info.get('DeviceNode') != whole[0] or info.get('TotalSize') != expected_bytes or
            volume_info.get('VolumeUUID') != str(expected_uuid).upper() or volume_info.get('Writable') is not False or
            volume_info.get('DeviceNode') not in [e['dev-entry'] for e in entities]):
        raise SystemExit('Only the actual matching read-only Disk Image/volume is accepted')
    disk = Path('/dev/r' + Path(whole[0]).name)
    if not stat.S_ISCHR(disk.stat().st_mode):
        raise SystemExit('Require the verified virtual-media raw character device')

    spec = importlib.util.spec_from_file_location('verified_manifest', root / 'kits/VerifiedRoot/tools/manifest.py')
    manifest = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(manifest)
    keys = root / 'build/verified-root-host/keys'
    with disk.open('rb', buffering=1024 * 1024) as stream:
        record = manifest.make(stream, info['TotalSize'], keys / 'owner.pem', keys / 'owner-public.pem',
                               expected_uuid, 1, out)

    class View(ctypes.Structure):
        _fields_ = [('hashes', ctypes.c_void_p), ('image_bytes', ctypes.c_uint64),
                    ('block_count', ctypes.c_uint64), ('generation', ctypes.c_uint64),
                    ('volume_uuid', ctypes.c_uint8 * 16), ('image_sha256', ctypes.c_uint8 * 32)]

    native = ctypes.CDLL(str(root / 'build/verified-root-host/verified-root.dylib'))
    native.vr_host_open.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p,
                                    ctypes.c_uint64, ctypes.POINTER(View)]
    native.vr_host_block.argtypes = [ctypes.POINTER(View), ctypes.c_uint64, ctypes.c_void_p, ctypes.c_size_t]
    encoded = ctypes.create_string_buffer((out / 'manifest.bin').read_bytes())
    anchor = ctypes.create_string_buffer((out / 'trusted-key.bin').read_bytes())
    view = View()
    if native.vr_host_open(encoded, len(encoded) - 1, anchor, 1, ctypes.byref(view)):
        raise SystemExit('Real C signature verification failed')
    if bytes(view.volume_uuid) != expected_uuid.bytes or view.block_count != expected_blocks or view.image_bytes != expected_bytes:
        raise SystemExit('Real C volume identity/size differs from the expected image')

    complete = hashlib.sha256()
    indices = [0, 1, view.block_count // 2, view.block_count - 1]
    samples, lengths = [], []
    with disk.open('rb', buffering=1024 * 1024) as stream:
        for index in range(view.block_count):
            block = stream.read(min(4096, view.image_bytes - index * 4096))
            if native.vr_host_block(ctypes.byref(view), index, block, len(block)) != 0:
                raise SystemExit('Actual C block verification failed at ' + str(index))
            complete.update(block)
            if index in indices:
                samples.append(block.ljust(4096, b'\0'))
                lengths.append(len(block))
        if stream.read(1):
            raise SystemExit('Virtual media exceeded its authenticated exact bounds')
    if complete.digest() != bytes(view.image_sha256) or complete.hexdigest() != record['image_sha256']:
        raise SystemExit('Actual complete volume digest differs')
    if complete.hexdigest() != expected_sha:
        raise SystemExit(f'Volume digest {complete.hexdigest()} != expected {expected_sha}')
    (out / 'original-samples.bin').write_bytes(b''.join(samples))
    report = dict(image_bytes=view.image_bytes, blocks_verified=view.block_count,
                  image_sha256=complete.hexdigest(), volume_uuid=str(expected_uuid).upper(),
                  generation=view.generation, source_compressed_sha256=source_digest,
                  source_format=args.source_format, host_bus_protocol='Disk Image',
                  host_media_writable=False, trusted_key_sha256=record['trusted_key_sha256'],
                  actual_sample_indices=indices, actual_sample_lengths=lengths,
                  real_c_signature_and_block_verification=True, original_image_modified=False,
                  host_only=True, boot_root_authenticated=False, qemu_tested=False,
                  hardware_tested=False)
    (out / 'verification.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS: {args.component} signature and all {view.block_count} blocks verified by real C/RSA')


if __name__ == '__main__':
    main()
