# Marble ShengUfs fork

Upstream: `https://github.com/UniXNU/ShengUfs.git` at
`e2b32cc429e8cf1501161c1d8c517f8994ceb9b2`.

This copy is the board fork for SM7475. It is not selected by
`marble_defconfig` in the first-console stage; select `ShengUfs` in
`board.toml [components]` and `CONFIG_XNU_NATIVE_UFS=y` when the verified-root
inputs exist.

## Changes

1. **Apps-SMMU stream override.** The vendor marble DT routes the UFS host
   through stream `0xE0`; sheng uses `0x60`. `ufsStreamId()` defaults to
   `0xE0`, accepts `native-ufs-stream=<hex>`, and is used by both
   `smmuBypassStream` call sites and the SMR match diagnostic.
2. **Probe-only build.** `timerTicks()` and `native_ufs_read_ticks` are
   defined outside the `NATIVE_VERIFIED_BLOCK` guard. The read path uses them
   even when the verified-root bundle is not linked, so the upstream source
   does not compile in probe-only mode without this move.
3. **Firmware UFS bank takeover.** SM7475's UEFI leaves the UFS stream mapped
   by an exclusive SMR (`0x800000e0`, mask 0) to context bank 2 with
   translation (`s2cr=2`). `native-ufs-smmu-takeover=1` lets the planner
   accept that exclusive bank; `configureUfsSmmu()` clears SCTLR and sets
   CBAR to the driver's bypass state before using it. The retry range is the
   full context-bank count, because the firmware bank can be far below the
   top eight (marble uses bank 2).

Everything else is upstream at the pinned revision. The UFS host, GCC, PHY
and SMMU register constants are the SM8550 values the driver already carries;
they match the marble vendor DT, but they have not been exercised on marble.
