# Marble hardware inventory

Sources (the upstream `chibishizuku/Mu-Silicium-marble` checkout, locally
`uefi-marble`):

- `uefi-marble/Resources/DTBs/marble.dts` and `marble.dtb`, the stock
  crDroidAndroid-15.0 marble device tree used by the UEFI port.
- `uefi-marble/Resources/DTBs/marble-unixnu.dtb`, the boot-image copy
  with the stale `/chosen` initrd properties removed.
- `uefi-marble/Platforms/Xiaomi/marblePkg/Library/MemoryMapLib/MemoryMapLib.c`
  and `ConfigurationMapLib/ConfigurationMapLib.c`.
- `uefi-marble/Resources/Configs/marble.toml`.
- `sing0626/UniXNU-marble` (legacy port) hardware notes, used only for
  cross-checks.

No serial number, seed, MAC address or owner data is included.

## Board identity

| Item | Value |
| --- | --- |
| Marketing names | Redmi Note 12 Turbo (China) / POCO F5 (Global) |
| Codename | `marble` |
| SoC | Qualcomm SM7475-AB, Snapdragon 7+ Gen 2 (Cape family) |
| CPU | 4x Cortex-A510, 3x Cortex-A710, 1x Cortex-X2 |
| GPU | Adreno 725 (not implemented) |
| Display | 1080x2400 AMOLED, panel-dependent |
| Touch | Goodix GT9916S, SPI |
| Storage | UFS 3.1, LUN 0 |
| Firmware DTB | `qcom,ukee-mtp`, `qcom,msm-id = <0x24f 0x10000>`, `qcom,board-id = <0x10008 0>` |

## CPU and interrupts

| Resource | Value |
| --- | --- |
| Boot CPU | affinity 0 |
| PSCI | `arm,psci-1.0`, method `smc` |
| GIC distributor | `0x17100000`, size `0x10000` |
| GIC redistributors | `0x17180000`, size `0x200000` |
| Redistributor stride | `0x40000` |
| Timer | 19.2 MHz, virtual timer PPI 11 (INTID 27) |

The board DTS tags cpu0-3 `E` and cpu4-7 `P`. The SoC patch series adds
`MAX_CPUS=8`, `MAX_CPU_CLUSTERS=2` and the E/P scheduler policy.

## Memory map

`MemoryMapLib.c` describes the UEFI view. Key regions:

| Region | Base | Size |
| --- | --- | --- |
| DDR base | `0x80000000` | board-dependent |
| HYP | `0x80000000` | `0x00600000` |
| DT BLOB | `0x80600000` | `0x00040000` |
| AOP | `0x80800000` | `0x000A0000` |
| SMEM | `0x80900000` | `0x00200000` |
| PIL reserved | `0x85200000` | `0x1C200000` |
| Display Demura | `0xA1400000` | `0x02B00000` |
| UEFI FD | `0xA7100000` | `0x00300000` |
| Kernel (firmware) | `0xA8000000` | `0x10000000` |
| Display reserved | `0xB8000000` | `0x02B00000` |
| OEM VM | `0xBB000000` | `0x05000000` |
| MTE reserved | `0xC0000000` | `0x20000000` |
| VM PIL | `0xE0000000` | `0x05700000` |
| TZ apps | `0xEA000000` | `0x07400000` |
| DXE heap | `0xF8000000` | `0x07C00000` |

The stock 12 GiB DTB reports six banks (`0x80000000`, `0xf4600000`,
`0x800000000`, `0x840000000`, `0x900000000`, `0x837b00000`). The board DTS
declares a single 2 GiB handoff range at `0x80000000`; the payload requests a
separate 4 GiB contiguous `owned_memory_size` window. DarwinBoot places that
window in actual free memory, so the 8 GiB SKU must be checked before raising
it.

## Storage

| Resource | Value |
| --- | --- |
| UFS host | `0x1d84000`, size `0x3000` |
| UFS ICE | `0x1d88000`, size `0x8000` |
| UFS PHY/ICE-HWKM | `0x1d90000`, size `0x9000` |
| UFS SPI | 265 (`0x109`) |
| Apps SMMU | `0x15000000`, size `0x100000` (second region `0x151ce000/0x20`) |
| UFS apps-SMMU stream | `0xE0` (vendor DT `iommus = <&apps_smmu 0xe0 0>`) |
| GCC | `0x100000`, size `0x1f4200` |

The upstream ShengUfs driver programs stream `0x60` (sheng). The board fork in
`drivers/ShengUfs` defaults to `0xE0` and accepts
`native-ufs-stream=<hex>`. The controller, GCC and SMMU register offsets are
the SM8550 ones that the driver already carries; they must be re-read on the
phone before DMA is claimed.

Measured with the read-only UFS probe on 2026-10-31: the `userdata` GPT entry
is `first_lba=0x2f6a00`, `last_lba=0x3b037fa`, 4096-byte LBAs, 224.2 GiB. The
current content has no UniXNU `SHNGROOT` manifest (`manifest status=1`,
`windows=0`); it is the phone's Android ext4 userdata and will be erased when
the UniXNU userdata image is flashed.

The capture commands, for another SKU:

```sh
# On the phone (rooted Android or recovery):
adb shell 'cat /proc/partitions'
adb shell 'dd if=/dev/block/sda bs=4096 count=1 2>/dev/null' > /tmp/marble-gpt.bin
# Or from the bootloader:
fastboot getvar partition-size:userdata
```

`ShengUfs` reads the partition named `userdata` from LUN 0's primary GPT
(4096-byte LBAs). Fill `[userdata] first_lba` and `last_lba` from that entry.

## Display and touch

| Resource | Value |
| --- | --- |
| Panel active area | 1080x2400 |
| Splash / display reserved | `0xB8000000`, size `0x2B00000` |
| UEFI GOP | SimpleFbDxe, 1080x2400x32 |
| Touch controller | Goodix GT9916S, `compatible = "goodix,gt9916S"` |
| Touch SPI | `spi@990000`, size `0x4000`, SPI 605, CS0, 15 MHz max |
| Touch IRQ | TLMM GPIO21, flags `0x08` (`goodix,irq-flags = 2`) |
| Touch reset | TLMM GPIO20, active low |
| Touch panel range | `0x438` x `0x960` (1080 x 2400) |
| SPI clocks | GCC IDs 94 (`se-clk`), 134 (`m-ahb`), 135 (`s-ahb`) |
| GPI | `0x900000/0x60000`, TX0/RX1, SEID4, protocol1, 64 TREs |

The touch path needs a GENI SPI + GPI DMA + Goodix protocol driver and an
`IOHIDDevice` publisher. None is selected in the first-console configuration.

## UEFI and boot image

| Item | Value |
| --- | --- |
| UEFI config | `Resources/Configs/marble.toml` |
| UEFI FD base | `0xA7100000` |
| UEFI FD size | `0x300000` (3 MiB) |
| Boot header | Android boot image v0 (kernel `0x80008000`, 160 MiB ramdisk `0xA8000000-0xB2000000`) |
| Boot image kernel | gzip(BootShim + FD) + appended firmware DTB |
| Firmware DTB | `Resources/DTBs/marble-unixnu.dtb` (stock tree minus stale `/chosen` initrd properties; keeps `cont_splash_region`); SHA-256 `a338d301291ed3d44c587a4f6534c9d4ce71168174c7c16546d07158125ba05b` |
| Boot partition | 192 MiB (`0xC000000`) |

The firmware volume path is
`Build/marblePkg/{DEBUG,RELEASE}_CLANGPDB/FV/SILICIUM_UEFI.fd`.

## Firmware state observed by the legacy port

The historical `sing0626/UniXNU-marble` port reported:

- a temporary `fastboot boot` image reached the iOS Hello screen with a clean
  GOP-backed display;
- only one boot CPU was described;
- UFS/HFS reads progressed to `launchd`;
- physical touch never worked;
- no GPU, Wi-Fi, cellular, audio or power-management support was claimed.

Those observations are not reused as verification for this port. They only
confirm that the display and storage paths are reachable with user-supplied
inputs.
