#!/bin/bash
# macOS runner: build the owner-signed OS/SystemOS policies and an ephemeral
# APFS Data seed for the marble tail root.  Nothing here writes a device.
set -euo pipefail

UNIXNU="${1:?usage: macos-verified-root.sh UNIXNU_CHECKOUT DEVICE_CHECKOUT}"
DEVICE="${2:?usage: macos-verified-root.sh UNIXNU_CHECKOUT DEVICE_CHECKOUT}"
OPENSSL_PREFIX="${OPENSSL_PREFIX:-$(brew --prefix openssl@3)}"
CACHE="$UNIXNU/.cache/firmware/target-volumes"
OS_DMG="$CACHE/044-76040-038.dmg"
SYSTEMOS_DMG="$CACHE/044-76034-038.dmg"
OS_MOUNT="$CACHE/os-mount"
SYSTEMOS_MOUNT="$CACHE/systemos-mount"
OS_DEV=""
SYSTEMOS_DEV=""

cleanup() {
  if [ -n "$OS_DEV" ]; then hdiutil detach "$OS_DEV" -force >/dev/null 2>&1 || true; fi
  if [ -n "$SYSTEMOS_DEV" ]; then hdiutil detach "$SYSTEMOS_DEV" -force >/dev/null 2>&1 || true; fi
}
trap cleanup EXIT

mkdir -p "$CACHE" "$OS_MOUNT" "$SYSTEMOS_MOUNT"

echo "== fetch device feeds"
cd "$UNIXNU"
./unixnu fetch -D "$DEVICE"

echo "== fetch OS and SystemOS members"
./unixnu ipsw fetch -D "$DEVICE" OS Cryptex1,SystemOS
cp -f "$UNIXNU/out/ipsw/21H461/044-76040-038.dmg" "$OS_DMG"
cp -f "$UNIXNU/out/ipsw/21H461/044-76034-038.dmg" "$SYSTEMOS_DMG"

echo "== build owner host verifier and keys"
python3 "$UNIXNU/kits/VerifiedRoot/tests/check.py" \
  --output "$UNIXNU/build/verified-root-host" \
  --openssl-prefix "$OPENSSL_PREFIX"

echo "== attach read-only virtual media"
OS_DEV=$(hdiutil attach -readonly -nobrowse -mountpoint "$OS_MOUNT" "$OS_DMG" | awk '/^\/dev\/disk/ {print $1; exit}')
SYSTEMOS_DEV=$(hdiutil attach -readonly -nobrowse -mountpoint "$SYSTEMOS_MOUNT" "$SYSTEMOS_DMG" | awk '/^\/dev\/disk/ {print $1; exit}')
test -n "$OS_DEV" && test -n "$SYSTEMOS_DEV"
diskutil info "$OS_MOUNT" >/dev/null
diskutil info "$SYSTEMOS_MOUNT" >/dev/null

echo "== owner policy: OS"
python3 "$DEVICE/scripts/build-owner-volume-policy.py" \
  --unixnu "$UNIXNU" --component os \
  --source "$OS_DMG" --mountpoint "$OS_MOUNT" \
  --out "$UNIXNU/build/verified-root-os" \
  --volume-uuid 04769D26-2CA0-4825-B443-E2E5F88300F2 \
  --image-sha256 e5634d04a8ac35f024845f583394e0a35deb26a04cb0c00286a8674e25a9a4f2 \
  --image-bytes 6077580800 --blocks 1483785 \
  --source-sha256 3f99b859c9728379110b66657c7b15285bcb1925ad8b5726542e2cdcf8b1028d \
  --source-format UDIF

echo "== owner policy: SystemOS"
python3 "$DEVICE/scripts/build-owner-volume-policy.py" \
  --unixnu "$UNIXNU" --component systemos \
  --source "$SYSTEMOS_DMG" --mountpoint "$SYSTEMOS_MOUNT" \
  --out "$UNIXNU/build/verified-root-systemos" \
  --volume-uuid 9071148B-B025-402D-AD7A-48464D8B5FFF \
  --image-sha256 b4908cde41d23e6bc05e6af51a2c9eeb00cf0f863a6a18b1600c96a3fd70414f \
  --image-bytes 3303014400 --blocks 806400 \
  --source-sha256 b4908cde41d23e6bc05e6af51a2c9eeb00cf0f863a6a18b1600c96a3fd70414f \
  --source-format UDRW

echo "== Data seed: 32 MiB APFS base from the OS initial var tree"
python3 "$UNIXNU/scripts/prepare-owner-data-fixture.py" \
  --output "$UNIXNU/build/owner-data-fixture"

echo "== Data seed: grow to 3 GiB logical (ephemeral packed seed)"
python3 "$UNIXNU/scripts/seed-data-grow.py" --gib 3 \
  "$UNIXNU/build/owner-data-fixture/owner-data-32.cdr" \
  "$UNIXNU/build/data-3g.dbr"

echo "== collect artifacts"
ART="$DEVICE/artifacts/verified-root-data"
rm -rf "$ART"
mkdir -p "$ART/owner-data"
cp -R "$UNIXNU/build/verified-root-os" "$ART/"
cp -R "$UNIXNU/build/verified-root-systemos" "$ART/"
cp "$UNIXNU/build/owner-data-fixture/seed.json" "$ART/owner-data/seed.json"
cp "$UNIXNU/build/data-3g.dbr" "$ART/owner-data/data-3g.dbr"
cp "$UNIXNU/build/verified-root-host/keys/owner-public.pem" "$ART/owner-public.pem"

python3 - "$ART" <<'PY'
import hashlib
import json
import sys
from pathlib import Path

art = Path(sys.argv[1])
os = json.loads((art / 'verified-root-os/verification.json').read_text())
systemos = json.loads((art / 'verified-root-systemos/verification.json').read_text())
seed = json.loads((art / 'owner-data/seed.json').read_text())
data = art / 'owner-data/data-3g.dbr'
summary = {
    'os': {k: os[k] for k in ('image_sha256', 'volume_uuid', 'blocks_verified', 'image_bytes', 'trusted_key_sha256')},
    'systemos': {k: systemos[k] for k in ('image_sha256', 'volume_uuid', 'blocks_verified', 'image_bytes', 'trusted_key_sha256')},
    'data_seed': {'volume_uuid': seed['volume_uuid'], 'loose_bytes': seed['bytes'],
                  'packed_bytes': data.stat().st_size,
                  'packed_sha256': hashlib.sha256(data.read_bytes()).hexdigest()},
    'owner_public_sha256': hashlib.sha256((art / 'owner-public.pem').read_bytes()).hexdigest(),
    'host_only': True,
    'hardware_tested': False,
}
(art / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
print(json.dumps(summary, indent=2))
PY

echo "== artifacts"
find "$ART" -maxdepth 2 -type f -print
