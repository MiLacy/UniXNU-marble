# marble board package

This directory becomes `kits/marble` in the assembled build tree.

There are no board EFI drivers or kext bundles yet. The UEFI firmware
integration (UniXNU application, registrar, RAM-disk driver and DarwinBoot)
lives in the Mu-Silicium marble DSC/FDF, because the loader must be in the
firmware volume. The XNU-side storage fork is in `../drivers/ShengUfs`.

`ShengPlatformPower.cpp` is the guarded ApplePMGR placeholder used by AMFI;
`../scripts/build-platform-power.py` generates its original-ABI headers and
binds the original ApplePMGR addresses. It is a bring-up bridge, not a
functional power driver.

`unixnu.toml` is the package's fallback menu configuration. The payload step
writes the real `EFI/UniXNU/unixnu.toml` into the FAT ramdisk.
