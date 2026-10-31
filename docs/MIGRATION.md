# Migration from `ios-port-marble`

This port starts from the hardware research in
[`jebbranch/ios-port-marble`](https://github.com/jebbranch/ios-port-marble)
and rebuilds it as a Format-2 UniXNU device repository. It does **not** import
the legacy boot shim or the iOS 16 kernel tree. The target is the upstream
UniXNU combination: iPadOS 17.7.11 (21H461), XNU 10063.141.1, 4 KiB pages,
UEFI boot and DarwinBoot.

| Input | Legacy repository | This port |
| --- | --- | --- |
| Boot | `shim/` plus `fastboot boot` of a v4 image | Mu-Silicium BootShim, `UniXNU.efi`, `UniXNURamDiskDxe`, `DarwinBoot` |
| Firmware | none; shim jumped directly to XNU | `uefi-marble` fork, 3 MiB FD at `0xA7100000` |
| XNU | 8796.141.3, 16 KiB pages, iPhone X-derived ADT | 10063.141.1, 4 KiB pages, ADT generated from `dts/marble.dts` |
| Userland | iOS 16.7.16, HFS+ image at the userdata tail | iPadOS 17.7.11 OS/SystemOS image, owner-verified tail windows |
| Storage | `shim/ufs.c`, custom HFS+ root | `ShengUfs` board fork, SMMU takeover and `IOBlockStorageDevice` bindings |
| Display | shim framebuffer console, XNU framebuffer | `GopFramebuffer` plus the Marble `IOMobileFramebuffer` probe |
| Touch | bit-banged SPI and Goodix GT9916R driver in `bsd/dev/marble_touch.c` | **not integrated**; the legacy protocol is a reference for a future `IOHIDDevice` driver |
| Audio | ADSP/GLINK/GPR/APM experiments | **not selected**; no ADSP component or firmware inputs |
| GPU | kgsl-smmu/GMU probe only | **not selected**; the catalog A740 driver does not cover Adreno 725 |
| Haptics | PMIC HAP520 experiments | **not selected**; no board power/haptics service |

## What was reused

- Board identity, SoC resources, GIC and timer addresses, UFS host/PHY/SMMU
  stream, display geometry, splash reservation and touch wiring, all checked
  against the stock marble device tree in `uefi-marble`.
- The measured `userdata` GPT geometry (`0x2f6a00`–`0x3b037fa`, 4096-byte LBAs)
  and the observation that the partition held Android ext4 data.
- The legacy findings that Gunyah traps `CNTP_CTL_EL0` and that the ADT
  interrupt mapper needs `AAPL,phandle`; both became small device/SoC patches
  against the upstream XNU profile instead of a 217-patch iOS 16 series.
- The display/framebuffer handoff model: the firmware GOP remains the first
  console, and a board package registers the iOS framebuffer subclass after
  the prelinked family loads.

## What was deliberately replaced

- **ADT.** The legacy port shipped a patched iPhone X `d22ap.adt`. That is an
  Apple binary and is not redistributable. UniXNU generates the ADT from the
  board DTS and the selected OS profile.
- **Kernel.** The iOS 16 tree and its 16 KiB page model do not match the
  upstream iPadOS 17 profile. Reusing the 217 legacy patches would make the
  device repository unreviewable and would not satisfy the OS ABI.
- **Boot path.** RAM-only v4 `fastboot boot` was replaced by the standard
  Mu-Silicium firmware volume and Android v0 boot image. The Stage 1 image has
  no ramdisk; the payload image carries the FAT16 volume at `0xA8000000`.

## Current gap: touch

The legacy driver and the independent
[`FoxLinne/UniXNU_marble`](https://github.com/FoxLinne/UniXNU_marble) source
show a working GENI SPI + Goodix path, but neither is part of this tree. A
future component must provide:

1. GENI SPI `0x990000`, SE4, 15 MHz, CS0 and GPI DMA;
2. TLMM GPIO20 reset and GPIO21 IRQ;
3. Goodix GT9916R power/reset, firmware info and contact parsing;
4. an `IOHIDDevice` digitizer publisher and edge gestures.

Until then `porting.toml` keeps the input area `planned`, and Setup cannot be
completed by touch.

## Reproduce this port

```sh
git clone https://github.com/MiLacy/UniXNU-sm7475-common ../UniXNU-sm7475-common
git clone https://github.com/chibishizuku/Mu-Silicium-marble ../uefi-marble
git -C ../uefi-marble apply firmware/uefi-marble-integration.patch

cd ../UniXNU
./unixnu defconfig marble_defconfig -D ../UniXNU-marble
./unixnu fetch -D ../UniXNU-marble
./unixnu tree -D ../UniXNU-marble
./unixnu build firmware -D ../UniXNU-marble --uefi ../uefi-marble --edk2 ../uefi-marble/Mu_Basecore
./unixnu bootimg -D ../UniXNU-marble --uefi ../uefi-marble \
  --fd ../uefi-marble/Build/marblePkg/DEBUG_CLANGPDB/FV/SILICIUM_UEFI.fd \
  --dtb ../uefi-marble/Resources/DTBs/marble-unixnu.dtb --ramdisk none
```

The kernel, payload and userdata stages additionally need a locally supplied
21H461 IPSW, matching trust caches, a Data seed and a unique chip id. None of
those files are in this repository.

## Source revisions

| Repository | Revision |
| --- | --- |
| `jebbranch/ios-port-marble` | `59567014a98d980cd13573bbb5b5932064571e18` |
| `chibishizuku/Mu-Silicium-marble` | `fc9e410d025cd9bd8cc3af7cdd450761e8ab6429` |
| `uefi-marble` integration | `d1f18525135544426fae87c9497155df7f9e1fc7` |
| `MiLacy/UniXNU-sm7475-common` | `fc78deda27304248186fec2320c5678c45d8e431` |

The legacy repository is Apache-2.0. Files copied from it, if any, keep that
notice; no Apple or Qualcomm binary is included. This port's own files are
BSD-2-Clause-Patent (see `LICENSE`).
