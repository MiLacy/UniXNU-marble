# uefi-marble integration

`board.toml [firmware]` builds a Mu-Silicium checkout at the paths below. This
directory records the minimal integration that turns the stock marble port into
a UniXNU firmware and keeps it reproducible from the URL named by the port.

| Item | Value |
| --- | --- |
| Upstream | `https://github.com/chibishizuku/Mu-Silicium-marble.git` |
| Upstream commit | `fc9e410d025cd9bd8cc3af7cdd450761e8ab6429` |
| Integration commit | `d1f18525135544426fae87c9497155df7f9e1fc7` |
| Branch | `unixnu-marble` |
| Patch | `uefi-marble-integration.patch` |
| Firmware config | `Resources/Configs/marble.toml` |
| DSC / FDF | `Platforms/Xiaomi/marblePkg/marble.dsc`, `marble.fdf` |
| FD output | `Build/marblePkg/{DEBUG,RELEASE}_CLANGPDB/FV/SILICIUM_UEFI.fd` |

## Prepare the checkout

```sh
git clone --recursive https://github.com/chibishizuku/Mu-Silicium-marble.git uefi-marble
cd uefi-marble
git checkout fc9e410d025cd9bd8cc3af7cdd450761e8ab6429
git apply ../UniXNU-marble/firmware/uefi-marble-integration.patch
# The patch adds a binary DTB; confirm it applied cleanly.
git status --short
```

If the upstream checkout is already a fork, cherry-pick the integration commit
instead of applying the patch. The patch is generated from
`fc9e410d..d1f18525` and is not meant to apply to a newer Mu-Silicium tree
without review.

## What the integration changes

| File | Change |
| --- | --- |
| `Platforms/Xiaomi/marblePkg/marble.dsc` | `UNIXNU_PAYLOAD=1`; map `UniXNULib`; reuse the `FdtLib`/`HobLib` mappings from `SiliciumPkg.dsc.inc` |
| `Silicon/Qualcomm/PalimaPkg/PalimaPkg.dsc.inc` | add `UniXNU`, `UniXNUDxe`, `UniXNURamDiskDxe` and `DarwinBoot` under the define |
| `Platforms/Xiaomi/marblePkg/marble.fdf` | place the four modules in `FVMAIN` |
| `Silicon/Silicium/SiliciumPkg/Sec/AArch64/ModuleEntryPoint.S` | preserve the bootloader FDT pointer in `x19` |
| `Silicon/Silicium/SiliciumPkg/Sec/Sec.c` | publish the pointer as entry-argument HOB `DAEE82E6-8090-4E72-BFB9-78C13CD4E359` |
| `BootShim/BootShim.S` | put the shim-plus-FD size in the ARM64 image header |
| `Resources/DTBs/marble-unixnu.dtb` | stock `marble.dtb` without stale `/chosen` initrd properties |

`DiskIoDxe`, `PartitionDxe`, `Fat` and `EnglishDxe` are already in
`Platforms/Xiaomi/marblePkg/Include/DXE.inc`; the integration does not duplicate
them.

## Build and check

From the sibling UniXNU checkout:

```sh
./unixnu build firmware -D ../UniXNU-marble \
  --uefi ../uefi-marble \
  --edk2 ../uefi-marble/Mu_Basecore

./unixnu bootimg -D ../UniXNU-marble \
  --uefi ../uefi-marble \
  --fd ../uefi-marble/Build/marblePkg/DEBUG_CLANGPDB/FV/SILICIUM_UEFI.fd \
  --dtb ../uefi-marble/Resources/DTBs/marble-unixnu.dtb \
  --ramdisk none
```

Expected checks:

- `SILICIUM_UEFI.fd` is exactly `0x300000` bytes at base `0xA7100000`.
- `FVMAIN.inf` lists the four UniXNU file GUIDs:
  `9B665204-A133-47A2-9396-9E2B370A4E25` (UniXNU),
  `29B950CF-586D-49EA-9D71-F88645CCDAF6` (UniXNUDxe),
  `75CA63AD-FBAE-4A06-92A0-F937C671251D` (UniXNURamDiskDxe) and
  `1B64AF87-8415-4B57-9B16-680D70913B9F` (DarwinBoot).
- The entry-argument GUID bytes are present in `Sec.efi`.
- `boot-empty.img` is an Android v0 image with kernel at `0x80008000`,
  tags at `0x80000100`, no ramdisk, and a valid ARM64 shim header.

The DEBUG build used 99% of `FVMAIN` on 2026-11-01. If a later component
overflows it, build `RELEASE` or trim an unused vendor driver; do not enlarge
the FD beyond the `0xA7100000/0x300000` window without checking the memory map.
