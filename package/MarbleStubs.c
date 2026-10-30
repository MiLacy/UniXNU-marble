// SPDX-License-Identifier: BSD-2-Clause-Patent
// Weak definitions for optional components that the first-console kernel
// references but does not link. A static kernel link still requires a
// definition for every weak_import, so a zero-valued stub answers "not
// present" without changing any selected driver.
#include <stdint.h>

__attribute__((weak)) void boot_progress_poll(void);

__attribute__((weak)) void boot_progress_poll(void)
{
}

__attribute__((weak)) int native_ram_block_gpu_addon(const unsigned char **addon,
                                                      unsigned long long *bytes);
__attribute__((weak)) int native_ram_block_gpu_addon(const unsigned char **addon,
                                                      unsigned long long *bytes)
{
	(void)addon;
	(void)bytes;
	return 0;
}

__attribute__((weak)) volatile uint32_t native_ufs_dpu_state;
__attribute__((weak)) uint32_t native_ufs_dpu_map(unsigned slot, const uint64_t *pages,
                                                   unsigned count);
__attribute__((weak)) uint32_t native_ufs_dpu_map(unsigned slot, const uint64_t *pages,
                                                   unsigned count)
{
	(void)slot;
	(void)pages;
	(void)count;
	return 0;
}
