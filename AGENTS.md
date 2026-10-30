# Device agent guide

This repository owns the marble board DTS, component selection, firmware
inputs, partition policy, packaging and the real-device test record. The shared
SM7475 resources are in `../UniXNU-sm7475-common`; the UEFI platform is
`../Mu-Silicium-marble`.

## Read first

Read this README and these UniXNU pages before changing the port:

| Task | Required reading |
| --- | --- |
| Every port | [Porting](https://unixnu.com/porting/), [device repository](https://unixnu.com/porting/device-repository), [component catalog](https://unixnu.com/reference/components), UniXNU `components.toml` |
| Hardware | [Device tree](https://unixnu.com/porting/device-tree), [matching](https://unixnu.com/drivers/device-tree-matching), SM7475 `README.md` and `soc.toml` |
| Component | Its site page, README, catalog build command, dependencies and source |
| Kernel | [Source and patches](https://unixnu.com/xnu/source-and-patches), [patch reference](https://unixnu.com/xnu/patch-reference), selected OS profile |
| Images | [Prelink](https://unixnu.com/concepts/prelink), [payload](https://unixnu.com/concepts/payload), [UEFI](https://unixnu.com/porting/uefi-firmware) |
| Verification | [Building](https://unixnu.com/building), [bring-up](https://unixnu.com/porting/bring-up), [log capture](https://unixnu.com/porting/log-capture) |
| Migration | [Migration](https://unixnu.com/porting/migration) |

## Work

Search the catalog and inspect existing component interfaces before adding code.
Match hardware scope, OS ABI and board dependencies. Shared SM7475 resources
belong in `UniXNU-sm7475-common`; reusable drivers belong in their component
repositories. Preserve canonical component IDs and runtime identifiers.

The board fork of ShengUfs lives in `drivers/ShengUfs`. Its only functional
change from the catalog revision is the apps-SMMU stream override. Do not
silently replace upstream behavior; document every further change in
`drivers/ShengUfs/MARBLE.md` and the storage area of `porting.toml`.

Verify memory ranges, CPU affinities, interrupts, firmware hashes and partition
geometry from board evidence. Do not fill unknown values with guesses. Keep
`[userdata]` absent until the phone's GPT is measured.

Run from the main UniXNU checkout:

```sh
./unixnu defconfig marble_defconfig -D ../UniXNU-marble
./unixnu fetch -D ../UniXNU-marble
./unixnu status --json -D ../UniXNU-marble
./unixnu tree -D ../UniXNU-marble
./unixnu build kernel -D ../UniXNU-marble
./unixnu build firmware -D ../UniXNU-marble --uefi ../Mu-Silicium-marble --edk2 ../Mu-Silicium-marble/Mu_Basecore
./unixnu bootimg -D ../UniXNU-marble --uefi ../Mu-Silicium-marble --dtb ../Mu-Silicium-marble/Resources/DTBs/marble-unixnu.dtb --ramdisk none
```

Record commands, revisions, output paths, results and unresolved inputs.

## Real-device prompts

The agent cannot test hardware. After changing a stage, ask the user to test
the matching stage on a physical marble and point them at
`docs/REAL-DEVICE-TESTS.md`:

1. **UniXNU.efi start:** does the firmware reach the UniXNU menu and log
   `UNIXNU:` lines?
2. **XNU kernel start:** does DarwinBoot validate the payload and does the
   kernel print its version and platform-expert nodes?
3. **System entry:** does the root mount, does `launchd` start, and does the
   display show the boot progress or Hello screen?

Capture the observation separately from the build result. A build that passes
is not a hardware result.

## Porting record

Read `porting.toml` and the [record guide](https://unixnu.com/porting/records)
before implementation. For storage, also read the
[UFS guide](https://unixnu.com/porting/ufs). Update each affected area's
findings or evidence with its code change. Run the checker before publishing:

```sh
python3 scripts/porting-record.py check --device ../UniXNU-marble
```

## Hardware adaptation

Follow [Port a device](https://unixnu.com/porting/) in order. Read the
[driver adaptation checklist](https://unixnu.com/porting/drivers) before selecting
components. Follow each subsystem guide for board inputs, implementation paths,
archive integration and expected results. Include board-package services such as
brightness, buttons, Wi-Fi, DSP, battery and CPU frequency in the porting record.

The display, PMGR, verified-root and Data work is intentionally staged. Read
`docs/FULL-SYSTEM.md` before enabling any of it. Do not enable a switch whose
archive and policy inputs do not exist.
