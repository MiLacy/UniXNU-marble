# Full-system roadmap

`marble_defconfig` builds the full UFS root kernel. It selects `ShengUfs`,
links the block-storage class against the original `IOBlockStorageDevice` ABI
and passes `native-ufs-root=1`, but the root is **unverified**: the
verified-root bundle, the owner-root policy and the Data seed are not built.
Reaching SpringBoard on marble needs the same userland chain sheng and alioth
use, with apples-to-apples inputs. This page lists the switches, archives and
board files in the order they should be enabled. Do not turn on a switch whose
archive or policy input does not exist; the kernel build checks some of them
and fails at the link.

## 1. Storage root

The current image uses the original IOStorageFamily base class directly. The
non-probe UFS block class subclasses `IOBlockStorageDevice`, whose vtable,
constructors, destructors, metaclass and inherited methods live in the
prelinked IOStorageFamily kext.

| Item | Value |
| --- | --- |
| `CONFIG_XNU_NATIVE_UFS=y` | `drivers/ShengUfs`, board fork `e2b32cc` |
| `CONFIG_XNU_NATIVE_UFS_FULL=y` | selects `NativeUfsBlock` and the storage bindings |
| `native-ufs-root=1` | publishes the userdata `OS`/`SystemOS` windows as root |
| `native-ufs-stream=0xe0` | SM7475 apps-SMMU stream |
| `native-ufs-smmu-takeover=1` | reuse the firmware's exclusive SMR bank 2 |
| `scripts/data/native-storage-original-bindings.S` | absolute ABI bindings from the 21H461 prelink |
| `scripts/build-storage-bindings.py` | compiles and archives the bindings into `libShengUfs.a` |

The binding addresses come from the reference `IOBlockStorageDevice.o`
(190 slots, 14 pure virtual) and the built `kernel.macho`/`kernel.json`
symbol scan. The table base (`0xfffffff005cb44c0`) still matches the
`__DATA_CONST` address in the full kernel, and `NativeUfsBlock` is present in
the linked kernel.

### Verified root (not built)

The verified-root chain is the next step after the unverified root test:

| Switch | Archive / input |
| --- | --- |
| `CONFIG_XNU_VERIFIED_ROOT_PROBE=y` | `build/verified-root-os-xnu-bundle/libVerifiedRootProbe.a` |
| `CONFIG_XNU_OWNER_ROOT_AUDIT=y` | same bundle |
| `CONFIG_XNU_OWNER_ROOT_TRUST=y` | same bundle |
| `CONFIG_XNU_NATIVE_VIRTIO_BLOCK=y` | `build/virtio-mmio-block-verified-bundle/libVirtioMmioBlock.a` |
| `CONFIG_XNU_NATIVE_VIRTIO_BLOCK_ARCHIVE="{tree}/build/virtio-mmio-block-verified-bundle/libVirtioMmioBlock.a"` | built by `scripts/build-arm-driver.py` with `--verified-root-bundle` |
| `CONFIG_XNU_VERIFIED_ROOT_ARCHIVE="{tree}/build/verified-root-os-xnu-bundle/libVerifiedRootProbe.a"` | same |

The verified-root bundle is built from the selected OS/SystemOS images and the
trust caches of the 21H461 restore. The exact builder and policy inputs are
recorded in the sheng and alioth storage records; the same iPadOS profile is
used here. Do not enable these switches without the bundle and policy: the
build fails at the link or the kernel rejects the root at authentication.

### Userdata geometry

Fill `board.toml [userdata]` from the phone's GPT:

```toml
[userdata]
first_lba = 0x...
last_lba = 0x...
lba_bytes = 4096
windows = [
  { name = "OS", component = "OS" },
  { name = "SystemOS", component = "Cryptex1,SystemOS" },
]
```

Then:

```sh
./unixnu build userdata --ipsw /path/to/iPad_Pro_HFR_17.7.11_21H461_Restore.ipsw --dry-run
./unixnu build userdata --ipsw /path/to/iPad_Pro_HFR_17.7.11_21H461_Restore.ipsw
```

Inspect `userdata-root.json` and confirm the image fits the measured partition
before writing. Do **not** run `fastboot flash userdata`: the root image belongs
at the tail, not at partition offset 0. Use
[`scripts/write-userdata-tail.py`](scripts/write-userdata-tail.py) from
TWRP/recovery; it writes only LBA `59589627..61880314` and verifies that the
Android ext4 first block is unchanged.

## 2. Data volume

The Data chain needs a seed whose APFS volume UUID matches the kernel header:

| Switch | Value |
| --- | --- |
| `CONFIG_XNU_NATIVE_RAM_BLOCK=y` | RamBlock archive |
| `CONFIG_XNU_OWNER_DATA_MOUNT=y` | Data seed |
| `CONFIG_XNU_OWNER_UNION_SYSTEMOS=y` | SystemOS cryptex union |
| `CONFIG_XNU_OWNER_CACHE_MOUNT=y` | SystemOS cache |
| `CONFIG_XNU_OWNER_CACHE_STARTUP=y` | launchd dyld cache |
| `CONFIG_XNU_OWNER_DATA_VOLUME_UUID="<seed UUID>"` | from the seed report |

The sheng default `18AB5557-DC2D-469B-8758-855E60F5B0DF` is only correct if
the seed was made with that UUID. Measure the seed you actually use and update
`marble_defconfig`.

`[payload.adt] packed_data_ramdisk = true` and `persistent_data = true` go with
a persistent UFS Data window. For the first root boot, a small ephemeral seed
is enough to reach the UI; persistence is a separate milestone.

## 3. Display

The provenance-matched display path is the `GopFramebuffer` legacy stack used
by sheng and alioth. It needs:

- `drivers/GopFramebuffer/NativeLegacyDisplay.cpp` and
  `NativeGOPPresenter.cpp`;
- a board `package/LegacyDisplayProbe.cpp` that registers the classes and
  binds the instance to the firmware framebuffer;
- `scripts/build-gop-legacy-display.py` and a kernel compile record;
- `package/LinkStubs.c` for the stride wrapper and missing weak hooks;
- `CONFIG_XNU_NATIVE_LEGACY_DISPLAY=y`;
- kernel arguments `offline-legacy-register=1` and `offline-legacy-gop=1`.

The alioth repository (`FFT-infra/UniXNU-alioth`) contains a cleaned,
board-decoupled version of these files. When porting it here:

1. copy the display-only parts (`LegacyDisplayProbe.cpp`, `LinkStubs.c`,
   `build-gop-legacy-display.py`, `build-link-stubs.py`);
2. remove the alioth-specific calls (`native_hid_input_probe`,
   `native_gpu_probe`, `alioth_pcie_init`, `native_keystore_probe`,
   `touchProbeStart`) or provide weak stubs;
3. keep the original-class address checks; they are for the same iPadOS
   17.7.11 kernelcache;
4. add the archive to `[kernel] archives` before `build-link-stubs.py`;
5. set `CONFIG_XNU_EXTRA_LD_ARCHIVES="{tree}/build/link-stubs/libLinkStubs.a"`.

The marble panel is 1080x2400. `NativeLegacyDisplay::startHardware()` reads the
firmware GOP and `PE_Video`; it does not need panel timings. The splash region
is already in the firmware DTB.

## 4. PMGR placeholder

Unmodified AMFI panics `AMFI: No PMGR?` when no `ApplePMGR` provider exists.
Sheng and alioth publish a guarded placeholder (`ShengPlatformPower.cpp`) that
implements no hardware operation and stops on unimplemented power calls.
The placeholder is enabled in the current tree so AMFI can pass; it is not
a functional power backend. Its inputs are:

- `CONFIG_XNU_SHENG_PMGR=y`;
- `build-platform-power.py` in `[kernel] archives`;
- kernel argument `sheng-pmgr=2`;
- a reference kernel from the previous build (the first pass links a no-op
  stub; the next pass links the real bridge).

This is a bring-up workaround, not a power-management implementation.

## 5. Touch

The Goodix GT9916S path is not implemented. The hardware inputs are:

- GENI SPI `0x990000`, SPI 605, CS0, 15 MHz;
- GPI `0x900000`, TX0/RX1, SEID4, 64 TREs;
- TLMM GPIO20 reset, GPIO21 IRQ;
- GCC IDs 94/134/135;
- apps-SMMU GPI ring `0x5b6` and QUP payload `0x5a3`.

A touch driver needs a GENI SPI transport, a GPI DMA provider, the Goodix
protocol and an `IOHIDDevice` publisher. `QcomGeniSpi` and `ShengHidInput` are
the catalog pieces; the Goodix protocol is board code. Until it exists, Setup
cannot be completed with touch. The legacy `sing0626/UniXNU-marble` repository
contains protocol decoders and a GPI TRE builder that can seed the work, but
its DMA path was never completed.

## 6. GPU, Wi-Fi, audio, power

Not started. The Adreno 725 is not the A740 the catalog driver covers. Wi-Fi is
likely a QCN/ath12k or WCN part that must be identified from the vendor DT and
firmware. Audio needs ADSP/codec firmware and GPR/GLINK/SMEM. Battery and CPU
frequency need PMIC/RPMh work. Keep them out of the porting record until there
is a source-backed implementation and a device observation.

## Order of verification

1. Console kernel (verified on marble).
2. UFS register read + GPT + userdata partition (verified on marble; probe
   geometry recorded).
3. Unverified full UFS root + userdata image (built on host, 2026-11-01;
   awaiting the tail write and marble test).
4. Verified root + Data seed + launchd.
5. Display stack + Hello/SpringBoard.
6. Touch.
7. GPU/Wi-Fi/audio/power.

Record each result in `porting.toml` and update
[docs/REAL-DEVICE-TESTS.md](REAL-DEVICE-TESTS.md) with the observed stage.
