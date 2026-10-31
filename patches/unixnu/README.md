# UniXNU host patch

`0001-bootimg-existing-dtb-node.patch` applies to the main UniXNU checkout.
It teaches `scripts/unixnu/bootimg.py` that `fdtput -c` may return
`FDT_ERR_EXISTS` when the board adds properties to a node such as `/chosen`
that the firmware DTB already contains. Without it, `unixnu bootimg` stops
with `Error at 'chosen': FDT_ERR_EXISTS` on this board.

`0002-adt-aapl-phandle.patch` makes the ADT generator write `AAPL,phandle`
instead of the plain FDT `phandle`. IOKit's `AddPHandle` only records the Apple
spelling; without this, `interrupt-parent` resolves to NULL and
`IODTGetICellCounts` data-aborts at FAR 0. This was observed on marble at PC
`0xfffffff0078ec634`, ESR `0x96000005`.

```sh
cd ../UniXNU
git apply ../UniXNU-marble/patches/unixnu/0001-bootimg-existing-dtb-node.patch
git apply ../UniXNU-marble/patches/unixnu/0002-adt-aapl-phandle.patch
```

The patch was tested against the UniXNU checkout used for the 2026-11-01
build (`scripts/unixnu/bootimg.py` blob `259fb29`). Rebase it if upstream
changes `patch_dtb()`.
