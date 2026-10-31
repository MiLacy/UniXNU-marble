# UniXNU-marble

UniXNU port for the **Redmi Note 12 Turbo / POCO F5 (`marble`)**, Qualcomm
**SM7475** (Snapdragon 7+ Gen 2), targeting the default **iPadOS 17.7.11
(21H461)** profile.

This repository is a Format-2 device port. It supplies the board device tree,
component selection, UEFI firmware inputs, kernel configuration and the
real-device test procedure. Stage 2 (XNU entry) is device-verified; the UFS
root and the userland stages remain open.

## Status

| Stage | State | What runs |
| --- | --- | --- |
| 1. UEFI / UniXNU menu | verified on marble | `UniXNU.efi`, `UniXNUDxe`, `UniXNURamDiskDxe` and `DarwinBoot` are in the 3 MiB marble firmware volume |
| 2. XNU kernel entry | **device-verified 2026-11-01** | XNU 10063.141.1 console kernel with the SM7475 patches; it reaches `IOKitBSDInit.cpp:984` and stops at the expected missing root device |
| 3. System entry | built on host, awaiting device test | full UFS root build: the UFS controller, SMMU takeover and `NativeUfsBlock` are linked against the original `IOBlockStorageDevice` ABI, and an 8.8 GiB `OS`/`SystemOS` userdata image is written. The root is **unverified**: no verified-root bundle and no Data seed. See [FULL-SYSTEM.md](docs/FULL-SYSTEM.md) |

**Please test on a real marble at every stage.** The exact prompts, commands
and observations to capture are in [REAL-DEVICE-TESTS.md](docs/REAL-DEVICE-TESTS.md).

## Built artifacts (host, no device result)

The Stage 1 artifacts below were rebuilt on 2026-11-01 from the published
UEFI commit and the `UniXNU` checkout. EDK II embeds its build time, so a
rebuild produces a different FD hash; compare size, FV contents and
`boot.json`, not only the hash.

| Artifact | Size | SHA-256 |
| --- | --- | --- |
| Firmware FD, DEBUG | 3,145,728 | `7bce194a149ace272581f870a8baa16499ae985be4a2490454f71fb8b5d211a4` |
| `boot-empty.img` (Stage 1) | 1,835,008 | `8adf6380c2991c389b6c08cbfc86036543f2fa647c71c34e3d52523c69f891cd` |
| Firmware DTB | 649,254 | `a338d301291ed3d44c587a4f6534c9d4ce71168174c7c16546d07158125ba05b` |

The Stage 1 image is at
`../UniXNU/out/marble/ipados-17.7.11-21H461/marble/boot/boot-empty.img`;
`hardware_tested` is false in `boot.json`. The full commands and checks are
in [docs/BUILD-2026-11-01.md](docs/BUILD-2026-11-01.md). The earlier full-root
`boot.img`, `kernel.macho`, trust bundle and 8.8 GiB `userdata-root.img`
were produced by the previous workspace and are **not** part of this
checkout. Rebuild them with the external inputs listed below.

## XNU first-entry image (2026-11-01)

The first-entry console build uses
[`configs/marble-console_defconfig`](configs/marble-console_defconfig): platform
expert, CPU/GIC, timer, PMGR and the XNU framebuffer console only. It disables
UFS, verified root, Data and the display stack, so the exact 21H461 IPSW and a
trust cache are enough.

| Artifact | Size | SHA-256 |
| --- | --- | --- |
| `boot.img` | 169,607,168 | `d169cb16d8d9bb25b6184bd544ce0d831a6f01499f037ab1424972b67b696b7e` |
| `payload.img` | 167,772,160 | `4a416b4ef506ea69279cde3e7ebe5ffeacc9fbb3a77630f54b0d43612e36f1c9` |
| console `kernel.macho` | 51,073,024 | `5270a0b93bea8db7de01f56440f8142430df4395954bf18335f8f5806c32e6df` |
| patched payload kernel | 51,073,024 | `87d112816c1aeaea046510bcfa210de4cbc72d3696cef1fcef0f0077f83679a5` |
| trust cache | 61,356 | `7416ca4f8377146626bb284e2133c76bee6cb4e3ee0af5d648b186235a7323a7` |

The image passes `cpus=1` and disables UFS root arguments for the first entry.
The complete bundle, including the FD and ADT, is in
`artifacts/marble-xnu-console/` in the workspace; see its README. The build is
host-only: `hardware_tested` is false until the phone is booted.

```sh
fastboot boot artifacts/marble-xnu-console/boot.img
```

## Hardware summary

| Input | Value | Source |
| --- | --- | --- |
| SoC | SM7475-AB, 4x A510 + 3x A710 + 1x X2 | stock marble DT |
| GIC | dist `0x17100000/0x10000`, redist `0x17180000/0x200000`, stride `0x40000` | stock marble DT |
| Timer | 19.2 MHz, virtual PPI 11 | stock marble DT |
| UFS | host `0x1d84000/0x3000`, PHY `0x1d80000/0x2000`, SPI 265 | stock marble DT |
| UFS apps-SMMU stream | `0xE0` (vendor DT) | stock marble DT |
| Display | 1080x2400, GOP framebuffer from UEFI | Mu-Silicium marble DSC |
| Splash / display reserved | `0xB8000000 + 0x2B00000` | stock marble DT |
| Touch | Goodix GT9916S on SPI `0x990000`, IRQ GPIO21, reset GPIO20 | stock marble DT |
| Firmware DTB | `Resources/DTBs/marble-unixnu.dtb` in uefi-marble | uefi-marble |
| UEFI FD | base `0xA7100000`, size `0x300000` | `Resources/Configs/marble.toml` |

The full inventory, including the capture commands, is in
[docs/HARDWARE.md](docs/HARDWARE.md).

## Repositories

| Repository | Path in this workspace | Role |
| --- | --- | --- |
| [UniXNU](https://github.com/UniXNU/UniXNU) | `../UniXNU` | build driver, ARM64 profile, OS profile, UEFI package |
| [`UniXNU-sm7475-common`](https://github.com/MiLacy/UniXNU-sm7475-common) | `../UniXNU-sm7475-common` | shared SM7475 DTSI and SMP/cluster patches |
| `UniXNU-marble` | this directory | board configuration, DTS, package and test record |
| `uefi-marble` | `../uefi-marble` | fork of `chibishizuku/Mu-Silicium-marble` with the UniXNU integration, marble DSC/FDF and firmware DTB |

`drivers/ShengUfs` is a board fork of the catalog component at
`e2b32cc429e8cf1501161c1d8c517f8994ceb9b2`. Its changes are the apps-SMMU
stream override: SM7475 uses `0xE0`, while the SM8550 driver programs `0x60`,
and the `native-ufs-smmu-takeover=1` path that reuses the firmware's
exclusive SMR/context bank 2 (the SM8550 layout). The boot arguments
`native-ufs-stream=0xe0` and `native-ufs-smmu-takeover=1` select marble.

The full (non-probe) block class subclasses the original
`IOBlockStorageDevice`, which lives in the prelinked IOStorageFamily kext.
`scripts/data/native-storage-original-bindings.S` binds that ABI (vtable,
constructors, destructors, metaclass and the inherited method slots) from the
21H461 prelink; `scripts/build-storage-bindings.py` compiles and archives it
into `libShengUfs.a` when `XNU_NATIVE_UFS_FULL=1`. The addresses were
recovered from the reference `IOBlockStorageDevice.o` (190 slots, 14 pure
virtual) and the built `kernel.macho` symbol table.

## Build

Run from the main UniXNU checkout. The UEFI checkout and Mu_Basecore paths are
local build inputs. Apply
[`patches/unixnu/0001-bootimg-existing-dtb-node.patch`](patches/unixnu/0001-bootimg-existing-dtb-node.patch)
to a stock UniXNU tree first; it lets `bootimg` add the fixed `/chosen` range
to the existing firmware DTB node.

```sh
cd ../UniXNU
./unixnu defconfig marble_defconfig -D ../UniXNU-marble
./unixnu fetch -D ../UniXNU-marble
./unixnu status --json -D ../UniXNU-marble
./unixnu tree -D ../UniXNU-marble
```

Firmware and the first boot image:

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

The empty-ramdisk image boots the firmware and the UniXNU menu. It is the
Stage 1 test.

Kernel and payload for first entry (IPSW and trust cache required):

```sh
./unixnu defconfig marble-console_defconfig -D ../UniXNU-marble
./unixnu tree -D ../UniXNU-marble
./unixnu build kernel -D ../UniXNU-marble \
  --ipsw ../iPad_Pro_HFR_17.7.11_21H461_Restore.ipsw

./unixnu build payload -D ../UniXNU-marble --uefi ../uefi-marble \
  --kernel out/marble/ipados-17.7.11-21H461/marble-console/kernel/kernel.macho \
  --trust-cache out/marble/ipados-17.7.11-21H461/marble/tree/build/marble-trust.bundle \
  --unique-chip-id 4d4152424c45 \
  --kernel-arg cpus=1 \
  --kernel-arg native-ufs-root=0 --kernel-arg native-ufs-stream=0 \
  --kernel-arg native-ufs-smmu-takeover=0 \
  --kernel-arg native-ufs-root-lba=0 --kernel-arg native-ufs-root-blocks=0

./unixnu bootimg -D ../UniXNU-marble --uefi ../uefi-marble \
  --fd ../uefi-marble/Build/marblePkg/DEBUG_CLANGPDB/FV/SILICIUM_UEFI.fd \
  --dtb ../uefi-marble/Resources/DTBs/marble-unixnu.dtb
```

Full UFS root kernel and payload (external inputs and verified-root bundle required):

```sh
# kernel: XNU source fetch and Linux/macOS toolchain
./unixnu build kernel -D ../UniXNU-marble

# the two original 21H461 trust modules, in the bundle format the loader accepts
./unixnu ipsw fetch -D ../UniXNU-marble StaticTrustCache Cryptex1,SystemTrustCache
python3 out/marble/ipados-17.7.11-21H461/marble/tree/scripts/make-marble-trust.py

# payload: kernel, ADT and trust cache; the current build has no Data seed
./unixnu build payload -D ../UniXNU-marble \
  --uefi ../uefi-marble \
  --trust-cache out/marble/ipados-17.7.11-21H461/marble/tree/build/marble-trust.bundle \
  --unique-chip-id 4d4152424c45

./unixnu bootimg -D ../UniXNU-marble \
  --uefi ../uefi-marble \
  --dtb ../uefi-marble/Resources/DTBs/marble-unixnu.dtb
```

The OS profile supplies the Apple iPad7,1 identity, kernelcache hash and
prelink roots. The board supplies the Qualcomm identity, memory window and
partition policy. The unique chip id in the example is a development seed;
replace it with a per-device value when installing.

## UEFI integration

`uefi-marble` is a fork of
[`chibishizuku/Mu-Silicium-marble`](https://github.com/chibishizuku/Mu-Silicium-marble)
at `fc9e410d`. The integration commit is `d1f18525` (branch
`unixnu-marble`); a standalone patch is in
[`firmware/uefi-marble-integration.patch`](firmware/uefi-marble-integration.patch).

- `Platforms/Xiaomi/marblePkg/marble.dsc` defines `UNIXNU_PAYLOAD=1` and maps
  `UniXNULib`. `FdtLib` and `HobLib` come from `SiliciumPkg.dsc.inc`.
- `Silicon/Qualcomm/PalimaPkg/PalimaPkg.dsc.inc` adds `UniXNU.inf`,
  `UniXNUDxe.inf`, `UniXNURamDiskDxe.inf` and `DarwinBoot.inf` under that
  define.
- `Platforms/Xiaomi/marblePkg/marble.fdf` places the same four modules in
  `FVMAIN`. `DiskIoDxe`, `PartitionDxe`, `Fat` and `EnglishDxe` were already
  present in `DXE.inc`.
- `Silicon/Silicium/SiliciumPkg/Sec/AArch64/ModuleEntryPoint.S` preserves the
  bootloader's `x0` in `x19`; `Sec.c` publishes it as the
  `DAEE82E6-8090-4E72-BFB9-78C13CD4E359` entry-argument HOB.
- `BootShim/BootShim.S` writes the full shim-plus-FD size into the ARM64
  image header so `unixnu bootimg` can verify the image.
- The four Mu-Silicium edk2 patches (`Auth-Service`, `Boot-Manager`, `Timer`,
  `Usb-Bus`) are applied by `board.toml [firmware] patches` for the build and
  reverted afterwards.

The boot-image DTB is `uefi-marble/Resources/DTBs/marble-unixnu.dtb`, derived
from the stock `marble.dtb` by deleting the stale `/chosen` initrd properties;
it keeps `cont_splash_region`. `[[bootimg.dtb_node]]` adds the fixed v0 initrd
range `0xA8000000-0xB2000000` to `/chosen`, matching the 160 MiB payload.

### Boot chain fixes (verified on marble)

The first hardware boot reported `no boot entry found`; the next boot reached
XNU, then AMFI stopped at `No PMGR?`. The seven fixes are:

1. **SEC entry argument.** `ModuleEntryPoint.S` keeps the bootloader's x0 in
   `x19` and passes it as the third `CEntryPoint` argument; `Sec.c` builds
   `gSiliciumEntryArgHobGuid` after `InitializeMemory`. Without it
   `UniXNURamDiskDxe` never sees the FDT.
2. **Payload location.** UniXNU resolves entry paths relative to
   `\EFI\UniXNU`, so the payload lives at
   `\EFI\UniXNU\Payloads\kernel.macho`; the generated `unixnu.toml` names
   `Payloads/kernel.macho`.
3. **FAT16 and controller binding.** The payload builder creates a valid
   FAT16 image (mformat's forced FAT32 BPB was rejected by EDK2 FAT).
   `UniXNURamDiskDxe` validates the FAT boot sector and calls
   `ConnectController` after publishing BlockIo; UniXNU connects all
   controllers before scanning for `unixnu.toml`.
4. **Boot image v0 with a fixed ramdisk.** `fastboot boot` does not reliably
   load the v4 ramdisk on marble. The board uses an Android v0 image with the
   kernel at `0x80008000`, the 160 MiB payload at `0xA8000000`, and the DTB
   `/chosen` initrd range `0xA8000000-0xB2000000` matching it.
5. **Gunyah physical-timer trap.** The panic came through XNU's `sleh` EL1
   vector, so the kernel was already at EL1; Gunyah nevertheless traps the
   EL1 `CNTP_CTL_EL0` write as UNDEF (`instr=d51be228`, PC
   `0xfffffff0074204fc`). The device patch
   `patches/xnu/marble-virtual-timer-only.patch` keeps the virtual timer
   enabled and removes the physical-timer write. The DarwinBoot EL2-to-EL1
   drop is retained as a defensive guard, but it was not the cause of this
   panic.
6. **ADT interrupt phandle.** XNU's `IODT` interrupt mapper looks up
   `AAPL,phandle`, but the FDT uses `phandle`; the UFS node's
   `interrupt-parent` therefore resolved to NULL and
   `IODTGetICellCounts()` data-aborted. The common ADT generator now emits
   `AAPL,phandle`, so the GIC is registered as an interrupt controller and
   the UFS interrupt mapping resolves.
7. **PMGR for AMFI.** AMFI panics `No PMGR?` without an `ApplePMGR` provider.
   The board package carries the sheng/alioth `ShengPlatformPower.cpp`
   placeholder; `CONFIG_XNU_SHENG_PMGR=y`, `sheng-pmgr=2` and
   `build-platform-power.py` publish the guarded provider. The bridge is
   built from the original kernelcache and the previous kernel; it implements
   no hardware power operation and panics if one is invoked.

The expected menu title is `UniXNU marble`; with this image the payload volume
is discovered and the `no boot entry found` notice is gone. The kernel
continues past `fiq_context_init` and the ADT interrupt map. With the
PMGR placeholder it should pass AMFI and stop at the missing root device.

The previous workspace produced a full UFS root `boot.img`
(`native-ufs-root=1`) paired with `userdata-root.img`; it is not included in
this checkout. The earlier read-only probe
(`native-ufs-probe=10`) recorded the geometry: `userdata` first `0x2f6a00`,
last `0x3b037fa` (4096-byte LBAs), no `SHNGROOT` manifest. Flashing the
userdata image overwrites Android user data; back up first. See
[Stage 3](docs/REAL-DEVICE-TESTS.md#stage-3--system-entry).

## Known gaps

- The root is **unverified**: `CONFIG_XNU_VERIFIED_ROOT_PROBE`,
  `CONFIG_XNU_OWNER_ROOT_AUDIT` and `CONFIG_XNU_OWNER_ROOT_TRUST` are off, so
  the kernel may stop at root authentication. There is no Data seed, so
  `/private/var` is expected to be missing after the root mounts. See
  [FULL-SYSTEM.md](docs/FULL-SYSTEM.md).
- The legacy GOP display stack is not enabled in `marble_defconfig`; the
  first console uses the XNU framebuffer console. Enabling it requires the
  board display probe and archive described in [FULL-SYSTEM.md](docs/FULL-SYSTEM.md).
- The PMGR placeholder is enabled for AMFI; it is not power management and
  panics on unimplemented power operations.
- Touch (Goodix GT9916S), GPU acceleration (Adreno 725), Wi-Fi, cellular,
  audio, camera and battery are not implemented.
- `[userdata]` geometry is measured (`0x2f6a00-0x3b037fa`), but the userdata
  image has been written only on the host; it has not been flashed to marble.

## Links

- [Port a device](https://unixnu.com/porting/)
- [Device repository](https://unixnu.com/porting/device-repository)
- [Device tree](https://unixnu.com/porting/device-tree)
- [UEFI firmware](https://unixnu.com/porting/uefi-firmware)
- [Bring-up](https://unixnu.com/porting/bring-up)
- [Component catalog](https://unixnu.com/reference/components)

`porting.toml` is the per-area record. Update it with every finding and run:

```sh
python3 scripts/porting-record.py check --device ../UniXNU-marble
```
