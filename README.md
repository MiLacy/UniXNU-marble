# UniXNU-marble

UniXNU port for the **Redmi Note 12 Turbo / POCO F5 (`marble`)**, Qualcomm
**SM7475** (Snapdragon 7+ Gen 2), targeting the default **iPadOS 17.7.11
(21H461)** profile.

This repository is a Format-2 device port. It supplies the board device tree,
component selection, UEFI firmware inputs, kernel configuration and the
real-device test procedure. It is not a claim of a completed hardware boot:
every stage below must be verified on a physical marble.

## Status

| Stage | State | What runs |
| --- | --- | --- |
| 1. UEFI / UniXNU menu | verified on marble | `UniXNU.efi`, `UniXNUDxe`, `UniXNURamDiskDxe` and `DarwinBoot` are in the 3 MiB marble firmware volume |
| 2. XNU kernel entry | verified on marble | XNU 10063.141.1 with the SM7475 SMP/cluster patches; payload carries the kernel, ADT and the two original trust caches |
| 3. System entry | built on host, awaiting device test | full UFS root build: the UFS controller, SMMU takeover and `NativeUfsBlock` are linked against the original `IOBlockStorageDevice` ABI, and an 8.8 GiB `OS`/`SystemOS` userdata image is written. The root is **unverified**: no verified-root bundle and no Data seed. See [FULL-SYSTEM.md](docs/FULL-SYSTEM.md) |

**Please test on a real marble at every stage.** The exact prompts, commands
and observations to capture are in [REAL-DEVICE-TESTS.md](docs/REAL-DEVICE-TESTS.md).

## Built artifacts (host, no device result)

| Artifact | SHA-256 |
| --- | --- |
| `boot-empty.img` (Stage 1) | `823a96d29602be5d2a678f8f9ecc054254b6f5c7482eaa3743be66069a6dc28d` |
| `boot.img` (full UFS root) | `ce8b5f6e57c716ecf0605839734bd2806d4ae470c83d96eb829842caff6147cc` |
| Firmware FD (3 MiB) | `6c52e8a4dcdd1483d7ca672de67725f8c76a60ececa956ba81d38a3aa4f7d4b4` |
| Firmware DTB | `58dca5dbe30e2d0f8ba599f3ff8d0c72b691ab07895cbf09807e67a136fbd2b3` |
| `kernel.macho` (full UFS root) | `c3ea36a2b438f2f84bca0e342699b9640dfb61a7631463576674a1b63a1795bd` |
| `userdata-root.img` (8.8 GiB) | `fd43ee7035ce185ce4215ac93fdc006fd784a54b3b7442e21457c226cbe03093` |
| Trust bundle | `7416ca4f8377146626bb284e2133c76bee6cb4e3ee0af5d648b186235a7323a7` |

The images are under
`../UniXNU/out/marble/ipados-17.7.11-21H461/marble/boot/` and
`.../marble/userdata/`. `hardware_tested` is false in `boot.json`: the full
UFS root image has not been run on marble yet.

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
| Firmware DTB | `Resources/DTBs/marble-unixnu.dtb` in Mu-Silicium-marble | Mu-Silicium |
| UEFI FD | base `0xA7100000`, size `0x300000` | `Resources/Configs/marble.toml` |

The full inventory, including the capture commands, is in
[docs/HARDWARE.md](docs/HARDWARE.md).

## Repositories

| Repository | Path in this workspace | Role |
| --- | --- | --- |
| [UniXNU](https://github.com/UniXNU/UniXNU) | `../UniXNU` | build driver, ARM64 profile, OS profile, UEFI package |
| [`UniXNU-sm7475-common`](https://github.com/MiLacy/UniXNU-sm7475-common) | `../UniXNU-sm7475-common` | shared SM7475 DTSI and SMP/cluster patches |
| `UniXNU-marble` | this directory | board configuration, DTS, package and test record |
| `Mu-Silicium-marble` | `../Mu-Silicium-marble` | UEFI platform, marble DSC/FDF and firmware DTB |

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
local build inputs.

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
  --uefi ../Mu-Silicium-marble \
  --edk2 ../Mu-Silicium-marble/Mu_Basecore

./unixnu bootimg -D ../UniXNU-marble \
  --uefi ../Mu-Silicium-marble \
  --fd ../Mu-Silicium-marble/Build/marblePkg/DEBUG_CLANGPDB/FV/SILICIUM_UEFI.fd \
  --dtb ../Mu-Silicium-marble/Resources/DTBs/marble-unixnu.dtb \
  --ramdisk none
```

The empty-ramdisk image boots the firmware and the UniXNU menu. It is the
Stage 1 test.

Kernel and payload (external inputs required):

```sh
# kernel: XNU source fetch and Linux/macOS toolchain
./unixnu build kernel -D ../UniXNU-marble

# the two original 21H461 trust modules, in the bundle format the loader accepts
./unixnu ipsw fetch -D ../UniXNU-marble StaticTrustCache Cryptex1,SystemTrustCache
python3 out/marble/ipados-17.7.11-21H461/marble/tree/scripts/make-marble-trust.py

# payload: kernel, ADT and trust cache; the current build has no Data seed
./unixnu build payload -D ../UniXNU-marble \
  --uefi ../Mu-Silicium-marble \
  --trust-cache out/marble/ipados-17.7.11-21H461/marble/tree/build/marble-trust.bundle \
  --unique-chip-id 4d4152424c45

./unixnu bootimg -D ../UniXNU-marble \
  --uefi ../Mu-Silicium-marble \
  --dtb ../Mu-Silicium-marble/Resources/DTBs/marble-unixnu.dtb
```

The OS profile supplies the Apple iPad7,1 identity, kernelcache hash and
prelink roots. The board supplies the Qualcomm identity, memory window and
partition policy. The unique chip id in the example is a development seed;
replace it with a per-device value when installing.

## UEFI integration

`Mu-Silicium-marble` was modified in place:

- `Platforms/Xiaomi/marblePkg/marble.dsc` lists `UniXNU.inf`,
  `UniXNUDxe.inf`, `UniXNURamDiskDxe.inf`, `DarwinBoot.inf` and the
  `UniXNULib`, `FdtLib` and `HobLib` library mappings.
- `Platforms/Xiaomi/marblePkg/Include/DXE.inc` places the four modules in the
  firmware volume. `DiskIoDxe`, `PartitionDxe`, `Fat` and `EnglishDxe` were
  already present.
- The four Mu-Silicium edk2 patches (`Auth-Service`, `Boot-Manager`, `Timer`,
  `Usb-Bus`) are applied by `board.toml [firmware] patches` for the build and
  reverted afterwards.

The boot-image DTB is
`Mu-Silicium-marble/Resources/DTBs/marble-unixnu.dtb`, derived from the stock
`marble.dtb` by deleting the stale `/chosen` initrd properties. It keeps
`cont_splash_region`. `[[bootimg.dtb_node]]` adds the fixed v0 initrd range
`0x80C00000-0x84C00000` to `/chosen`.

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
   kernel at `0x80008000`, the 64 MiB payload at `0x80C00000`, and the DTB
   `/chosen` initrd range matching it.
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

The current `boot.img` is the **full UFS root image** (`native-ufs-root=1`),
paired with `userdata-root.img`. The earlier read-only probe
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
