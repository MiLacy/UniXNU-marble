# Marble real-device tests

本文件是 marble 实机测试清单。构建结果不等于硬件结果；每个阶段都必须在
POCO F5 / Redmi Note 12 Turbo 上执行并记录。

The build host cannot execute any of these tests. After each stage, record:

- the exact image filename and SHA-256;
- the device SKU and RAM size;
- the last visible screen and the last log line;
- whether `fastboot devices` still lists the phone;
- the recovery action taken.

Use `fastboot boot`, never `fastboot flash boot`, for bring-up. A bad image
then disappears on the next reset instead of replacing the boot partition.

## Before you start

1. Back up anything on the phone. `userdata` is overwritten in Stage 3.
2. The bootloader must be unlocked; `fastboot boot` refuses a locked device.
3. Install Android platform tools and confirm `fastboot devices`.
4. Enter the bootloader:
   ```sh
   adb reboot bootloader
   fastboot devices
   ```
4. Keep the stock boot image and a known-good Android image available.
5. Build the Stage 1 image from the main UniXNU checkout:
   ```sh
   cd ../UniXNU
   ./unixnu defconfig marble_defconfig -D ../UniXNU-marble
   ./unixnu fetch -D ../UniXNU-marble
   ./unixnu tree -D ../UniXNU-marble
   ./unixnu build firmware -D ../UniXNU-marble \
     --uefi ../Mu-Silicium-marble \
     --edk2 ../Mu-Silicium-marble/Mu_Basecore
   ./unixnu bootimg -D ../UniXNU-marble \
     --uefi ../Mu-Silicium-marble \
     --fd ../Mu-Silicium-marble/Build/marblePkg/DEBUG_CLANGPDB/FV/SILICIUM_UEFI.fd \
     --dtb ../Mu-Silicium-marble/Resources/DTBs/marble-unixnu.dtb \
     --ramdisk none
   ```
   The payload image is written to
   `out/marble/ipados-17.7.11-21H461/marble/boot/boot.img`. The host build
   already produced both images; the empty one is `boot-empty.img` beside it.

---

## Stage 1 — UniXNU.efi starts

**请使用 marble 实机测试：执行下面的 `fastboot boot`，确认 UniXNU 菜单和
`UNIXNU:` 日志出现。**

```sh
fastboot boot out/marble/ipados-17.7.11-21H461/marble/boot/boot-empty.img
```

The built empty image SHA-256 is
`823a96d29602be5d2a678f8f9ecc054254b6f5c7482eaa3743be66069a6dc28d`.

Expected:

- The phone does not reset; the POCO / Redmi splash is replaced by the
  firmware framebuffer.
- The UniXNU menu is visible, or `UNIXNU:` lines appear on the serial log.
- The menu reports the entry, driver, patch, ACPI, slice and kit counts.
- With the empty ramdisk, UniXNU reports **`no boot entry found`**. That is
  the correct Stage 1 result: the image has no payload. It proves the
  firmware started UniXNU and parsed the configuration. Do not treat it as a
  boot failure; continue to Stage 2 with `boot.img`.

Capture:

- A photo of the screen and the last three visible lines.
- The serial log if a UART or USB CDC console is available.
- `fastboot getvar all` output if the phone resets (compare the active slot).

If the screen stays black or the phone resets:

1. Rebuild the firmware and compare the FD size with
   `Resources/Configs/marble.toml` (`0x300000`).
2. Check that `SILICIUM_UEFI.fd` contains `UniXNU.efi`, `UniXNUDxe.efi`,
   `UniXNURamDiskDxe.efi` and `DarwinBoot.efi`.
3. Check the boot image header is v0 with kernel `0x80008000` and ramdisk
   `0x80C00000`, and that the appended DTB is
   `Resources/DTBs/marble-unixnu.dtb` with `/chosen` initrd
   `0x80C00000-0x84C00000`.
4. Read [bring-up stage 1](https://unixnu.com/porting/bring-up).

Do not continue to Stage 2 until Stage 1 is repeatable.

---

## Stage 2 — XNU kernel starts

**请使用 marble 实机测试：启动带 payload 的 boot.img，确认 `DARWINBOOT:`
日志、内核版本行和平台专家节点出现。**

Inputs required (not in this repository):

- iPadOS 17.7.11 (21H461) restore image;
- matching kernelcache and trust cache;
- a 64-bit unique chip id;
- a Data seed only when testing the Data volume.

Build:

The host build already produced `boot.img` with the kernel, ADT and the two
original 21H461 trust modules. To rebuild it:

```sh
cd ../UniXNU
./unixnu build kernel -D ../UniXNU-marble
./unixnu ipsw fetch -D ../UniXNU-marble StaticTrustCache Cryptex1,SystemTrustCache
python3 out/marble/ipados-17.7.11-21H461/marble/tree/scripts/make-marble-trust.py
./unixnu build payload -D ../UniXNU-marble \
  --uefi ../Mu-Silicium-marble \
  --trust-cache out/marble/ipados-17.7.11-21H461/marble/tree/build/marble-trust.bundle \
  --unique-chip-id 4d4152424c45
./unixnu bootimg -D ../UniXNU-marble \
  --uefi ../Mu-Silicium-marble \
  --dtb ../Mu-Silicium-marble/Resources/DTBs/marble-unixnu.dtb
fastboot boot out/marble/ipados-17.7.11-21H461/marble/boot/boot.img
```

The Stage 2 result recorded with the first-console image was
`c615520c88e79e78d170bc1309b132a3f4465017cab46ae8f5bfd887cc2931b9`. The
current `boot.img` is the Stage 3 full UFS root image
`ce8b5f6e57c716ecf0605839734bd2806d4ae470c83d96eb829842caff6147cc`; use it
for the storage stage below.

Expected:

- `UNIXNU:` reports the payload path and the `xnu` entry.
- `DARWINBOOT:` prints the virtual base, physical base, arguments address,
  entry address, framebuffer base/width/height/depth and EL1.
- The kernel prints its XNU version and boot arguments.
- The platform expert publishes `/cpus`, `/arm-io`, `/defaults`, `/options`
  and the NVRAM proxy.

Capture:

- The last `DARWINBOOT:` line and the first kernel lines.
- The panic text if the kernel panics; the panic screen is enabled by
  `[payload] panic_screen = true`.
- Whether the kernel reached the platform expert before any driver attaches.

Common failures:

| Symptom | Check |
| --- | --- |
| panic at `fiq_context_init` with `instr=d51be228` | the kernel lacks `marble-virtual-timer-only.patch`; Gunyah traps the EL1 `CNTP_CTL_EL0` write. Rebuild with the current device patch series |
| data abort in `IODTGetICellCounts` | the ADT lacks `AAPL,phandle`; rebuild the payload with the current ADT generator |
| `AMFI: No PMGR?` | the PMGR bridge is missing; rebuild with `CONFIG_XNU_SHENG_PMGR=y` and `sheng-pmgr=2` |
| menu still says `no boot entry found` | the payload volume was not discovered; read the `UNIXNURAMDISK:` debug lines on screen |
| `UNIXNURAMDISK: no entry argument HOB` | the firmware was not rebuilt with the SEC entry-argument HOB; rebuild and reflash the FD |
| `UNIXNURAMDISK: x0 ... is not a device tree` | the bootloader did not pass the appended DTB in x0 |
| `UNIXNURAMDISK: no initrd in /chosen` | the bootloader did not publish the ramdisk; confirm `marble-unixnu.dtb` has no stale initrd properties and that the image has a ramdisk |
| `DARWINBOOT: boot refused` | kernel Mach-O, ADT sidecar, trust cache, placeholders |
| panic naming a provider | ADT node, driver archive, kernel flag |
| no kernel log | boot arguments, serial selection, framebuffer base |
| no display | use the framebuffer console; the legacy display stack is a later stage |

The Stage 2 image did not enable `native-ufs-root=1` or the display stack,
so the kernel reached the platform expert without touching UFS or the panel.
The current defconfig enables the full UFS root; Stage 2 above records the
historical first-console result.

Expected stop for the Stage 2 image: with the PMGR placeholder enabled, AMFI
and IOKit should pass; the kernel then has no root device. It waits for a root
and panics (or prints the root-device timeout). That was the correct result
for that image: Stage 2 proves kernel entry, platform setup and IOKit, not
userland. Stage 3 below is the root stage.

---

## Stage 3a — UFS read-only probe (completed)

The probe image selected `ShengUfs` in probe-only mode and passed
`native-ufs-probe=10`. It did not write userdata. Observed on marble
2026-10-31:

```text
NATIVE_UFS root check
userdata first=2f6a00 last=3b037fa
block0 magic a677e784 c7b59c23 ext4magic=c6cf ext4blocks=217367362
manifest status=1 windows=0
```

The earlier probe run also established the SMMU takeover: the firmware leaves
an exclusive SMR `0x800000e0` (slot 2) mapped to translating context bank 2
(`s2cr=2`, `id2=5111`). `native-ufs-smmu-takeover=1` reuses that bank after
clearing `SCTLR`/`CBAR`, and the retry range now covers all 128 context banks.
The geometry is in `board.toml [userdata]`; `manifest status=1 windows=0`
means the partition is the Android ext4 userdata, not a `SHNGROOT` image.

## Stage 3 — System entry

**请使用 marble 实机测试：先备份 Android 数据，刷入 userdata 镜像，再启动
完整 UFS root 镜像，确认根卷挂载、`launchd` 启动并记录最终的 panic 或启动
进度。**

The current full UFS root build is **unverified root without a Data seed**.
`CONFIG_XNU_VERIFIED_ROOT_PROBE`, `CONFIG_XNU_OWNER_ROOT_AUDIT`,
`CONFIG_XNU_OWNER_ROOT_TRUST` and the Data chain are off. Expected outcomes:

- best case: the root volume mounts and `launchd` starts, then stops because
  `/private/var` (Data) does not exist;
- likely: root authentication rejects the unverified OS/SystemOS images and
  the kernel panics with the exact authentication text to capture.

Inputs:

- `/home/lacy/WorkSpace/UniXNU/out/marble/ipados-17.7.11-21H461/marble/userdata/userdata-root.img`
  (9,382,658,048 bytes, SHA-256
  `fd43ee7035ce185ce4215ac93fdc006fd784a54b3b7442e21457c226cbe03093`);
- the matching full kernel `kernel.macho` SHA-256
  `c3ea36a2b438f2f84bca0e342699b9640dfb61a7631463576674a1b63a1795bd`;
- the current `boot.img` SHA-256
  `ce8b5f6e57c716ecf0605839734bd2806d4ae470c83d96eb829842caff6147cc`.

**Flashing `userdata` erases Android user data.** Back up first. `fastboot
boot` still only boots the image; the boot partition is not replaced.

```sh
adb reboot bootloader
fastboot devices
fastboot flash userdata /home/lacy/WorkSpace/UniXNU/out/marble/ipados-17.7.11-21H461/marble/userdata/userdata-root.img
fastboot boot /home/lacy/WorkSpace/UniXNU/out/marble/ipados-17.7.11-21H461/marble/boot/boot.img
```

Expected:

- UFS logs the link speed, command queue and apps-SMMU stream `0xE0`.
- `NativeUfsBlock` publishes the `OS` and `SystemOS` windows from the
  userdata image.
- The root volume UUID matches
  `04769D26-2CA0-4825-B443-E2E5F88300F2`.
- XNU mounts the root and starts `launchd`, or panics with the unverified-root
  authentication text.

Capture:

- The UFS log (`NativeUfs:` lines), the root-matching line and the first
  `launchd` line or the panic block.
- A photo of the screen at the last visible state.
- The `userdata` report (`userdata-root.json`) and the image SHA-256.

If the root does not mount:

1. Verify the GPT first/last LBA and the 4096-byte logical block size.
2. Confirm the data image was built from the same 21H461 IPSW.
3. Confirm `native-ufs-stream=0xe0`, `native-ufs-smmu-takeover=1` and
   `native-ufs-root=1` are in the packed command line.
4. If the panic names an unresolved IOStorageFamily method or a bad vtable,
   the storage bindings do not match the running prelink; rebuild with
   `XNU_NATIVE_UFS_FULL=1` and re-check the symbols listed in
   `scripts/data/native-storage-original-bindings.S`.
5. If authentication rejects the root, the next step is the verified-root
   bundle (`CONFIG_XNU_VERIFIED_ROOT_PROBE` and the owner-root audit/trust
   switches), not a kernel change.

---

## Stop conditions

Stop and reset into the bootloader if:

- the phone does not enumerate on USB after two minutes;
- the screen shows a kernel panic that names memory corruption;
- UFS reports DMA faults or the SMMU reports an unexpected stream;
- the phone becomes hot or the charger LED behaves abnormally.

Report the stage, image hash, last log line and the exact recovery command.
