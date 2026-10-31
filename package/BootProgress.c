// SPDX-License-Identifier: MIT
// Minimal on-screen boot progress for the marble bring-up: a framebuffer
// console for the kernel's own console output, explicit stage lines, per-exec
// lines and a once-per-second heartbeat with UFS counters. It exists so a
// headless boot can be told apart from a hang. The framebuffer mapping comes
// from initialize_screen() through boot_progress_bind().
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <mach/mach_time.h>
#include <kern/clock.h>
#include <kern/thread.h>
#include <kern/sched_prim.h>
#include <kern/sched.h>
#include "DiagFont.h"

#define BP_SCALE 3
#define BP_CELL  (6 * BP_SCALE)
#define BP_LINE  (14 * BP_SCALE)
#define BP_LOGO_SCALE 5
#define BP_PROGRESS_TOTAL 2290185ULL

static unsigned mLogY, mHeaderY, mBarY;

static const uint64_t mFont[] = GLYPH_FONT;

static uint8_t *mBase;
static unsigned mWidth, mHeight, mRowBytes;
static unsigned mX, mY;
static unsigned mCols;
static int mReady;
static unsigned mLock;
static uint64_t mStart;
static uint64_t mBeatInterval;
static volatile uint64_t mIrqCount, mBrkCount, mAbortCount;
static volatile unsigned mBrkEsr, mAbortEsr, mAbortFar;
static volatile int mBeat;

extern volatile uint64_t native_ufs_block_reads;
__attribute__((weak)) uint64_t sweet_fb_swap_count;
__attribute__((weak)) uint64_t sweet_fb_started;
__attribute__((weak)) uint64_t sweet_fb_clients;
__attribute__((weak)) uint64_t sweet_surf_calls;
__attribute__((weak)) uint32_t sweet_surf_last;
__attribute__((weak)) uint64_t marble_display_calls;
__attribute__((weak)) uint32_t marble_display_last;
__attribute__((weak)) uint32_t marble_display_counts[262];
__attribute__((weak)) uint64_t marble_display_started;
__attribute__((weak)) uint64_t marble_display_init;
extern volatile uint32_t armcpu_smp_fail[8];
extern volatile uint32_t armcpu_smp_step[8];
extern volatile uint32_t armcpu_core_timer[8];
extern uint32_t processor_avail_count;
extern unsigned int processor_count;
extern volatile int sheng_launch_policy_status;
static unsigned mStatusY;
extern volatile uint64_t native_ufs_verified_blocks;

static int
bp_lock (void)
{
	return __atomic_exchange_n (&mLock, 1, __ATOMIC_ACQUIRE) == 0;
}

static void
bp_unlock (void)
{
	__atomic_store_n (&mLock, 0, __ATOMIC_RELEASE);
}

static void
bp_fill (unsigned x, unsigned y, unsigned w, unsigned h, uint32_t color)
{
	if (!mReady) {
		return;
	}
	for (unsigned row = 0; row < h && y + row < mHeight; row++) {
		volatile uint32_t *line = (volatile uint32_t *)(mBase + (size_t)(y + row) * mRowBytes);
		for (unsigned col = 0; col < w && x + col < mWidth; col++) {
			line[x + col] = color;
		}
	}
}

static void
bp_glyph_s (unsigned x, unsigned y, char c, uint32_t fg, uint32_t bg, unsigned scale)
{
	uint64_t glyph;
	unsigned top, bottom;

	if (c < 32 || c > 126) {
		c = '?';
	}
	glyph = mFont[c - 32];
	top = (unsigned)(glyph >> 32);
	bottom = (unsigned)glyph;
	for (unsigned row = 0; row < 14 * scale; row++) {
		volatile uint32_t *line = (volatile uint32_t *)(mBase + (size_t)(y + row) * mRowBytes);
		for (unsigned col = 0; col < 6 * scale; col++) {
			if (x + col < mWidth) {
				line[x + col] = bg;
			}
		}
	}
	for (unsigned row = 0; row < 12; row++) {
		unsigned data = (row < 6) ? (top >> (row * 5)) : (bottom >> ((row - 6) * 5));
		for (unsigned col = 0; col < 5; col++) {
			if ((data >> col) & 1) {
				for (unsigned dy = 0; dy < scale; dy++) {
					if (y + row * scale + dy >= mHeight) {
						continue;
					}
					volatile uint32_t *line = (volatile uint32_t *)(mBase + (size_t)(y + row * scale + dy) * mRowBytes);
					for (unsigned dx = 0; dx < scale; dx++) {
						if (x + col * scale + dx < mWidth) {
							line[x + col * scale + dx] = fg;
						}
					}
				}
			}
		}
	}
}

static void
bp_glyph (unsigned x, unsigned y, char c, uint32_t fg, uint32_t bg)
{
	bp_glyph_s (x, y, c, fg, bg, BP_SCALE);
}

static void
bp_text_at (unsigned x, unsigned y, unsigned scale, const char *s, uint32_t fg)
{
	while (*s) {
		bp_glyph_s (x, y, *s++, fg, 0xFF000000u, scale);
		x += 6 * scale;
	}
}

static void
bp_text_centered (unsigned y, unsigned scale, const char *s)
{
	unsigned width = (unsigned)strlen (s) * 6 * scale;
	unsigned x = width < mWidth ? (mWidth - width) / 2 : 0;

	bp_text_at (x, y, scale, s, 0xFFFFFFFFu);
}

static void
bp_number_string (char *out, uint64_t value)
{
	char digits[24];
	unsigned n = 0, i = 0;

	do {
		digits[n++] = (char)('0' + value % 10);
		value /= 10;
	} while (value && n < sizeof (digits));
	while (n) {
		out[i++] = digits[--n];
	}
	out[i] = 0;
}

static void
bp_emit (char c)
{
	if (!mReady) {
		return;
	}
	if (c == '\n') {
		mX = 0;
		mY += BP_LINE;
	} else if (c == '\r') {
		mX = 0;
	} else if (c == '\t') {
		mX += 4 * BP_CELL;
	} else if (c >= 32 && c <= 126) {
		if (mX + BP_CELL > mWidth) {
			mX = 0;
			mY += BP_LINE;
		}
		bp_glyph (mX, mY, c, 0xFFFFFFFFu, 0xFF000000u);
		mX += BP_CELL;
	} else {
		return;
	}
	if (mY + BP_LINE > mHeight) {
		for (unsigned row = 0; row < mHeight - mLogY - BP_LINE; row++) {
			volatile uint32_t *dst = (volatile uint32_t *)(mBase + (size_t)(mLogY + row) * mRowBytes);
			volatile uint32_t *src = (volatile uint32_t *)(mBase + (size_t)(mLogY + BP_LINE + row) * mRowBytes);
			for (unsigned col = 0; col < mWidth; col++) {
				dst[col] = src[col];
			}
		}
		bp_fill (0, mHeight - BP_LINE, mWidth, BP_LINE, 0xFF000000u);
		mY = mHeight - BP_LINE;
	}
}

static void
bp_text (const char *s)
{
	while (*s) {
		bp_emit (*s++);
	}
}

static void
bp_number (uint64_t value, unsigned base)
{
	char digits[24];
	unsigned n = 0;

	do {
		unsigned d = (unsigned)(value % base);
		digits[n++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
		value /= base;
	} while (value && n < sizeof (digits));
	while (n) {
		bp_emit (digits[--n]);
	}
}

static unsigned
bp_progress (void)
{
	uint64_t verified = native_ufs_verified_blocks;

	if (verified >= BP_PROGRESS_TOTAL) {
		return 100;
	}
	return (unsigned)((verified * 100) / BP_PROGRESS_TOTAL);
}

static void
bp_bar (void)
{
	unsigned x0 = mWidth / 8, w = mWidth * 3 / 4, y = mBarY, pct = bp_progress ();
	unsigned fill = (w - 8) * pct / 100;
	char text[8];

	bp_fill (x0, y, w, 4, 0xFFFFFFFFu);
	bp_fill (x0, y + 24, w, 4, 0xFFFFFFFFu);
	bp_fill (x0, y, 4, 28, 0xFFFFFFFFu);
	bp_fill (x0 + w - 4, y, 4, 28, 0xFFFFFFFFu);
	bp_fill (x0 + 4, y + 4, w - 8, 20, 0xFF000000u);
	if (fill) {
		bp_fill (x0 + 4, y + 4, fill, 20, 0xFFFFFFFFu);
	}
	bp_number_string (text, pct);
	{
		char line[16];
		unsigned i = 0;
		while (text[i]) {
			line[i] = text[i];
			i++;
		}
		line[i++] = '%';
		line[i] = 0;
		bp_text_centered (y + 36, BP_SCALE, line);
	}
}

static void
bp_splash (void)
{
	const char *logo = "UniXNU";
	unsigned scale = BP_LOGO_SCALE;
	unsigned width = (unsigned)strlen (logo) * 6 * scale;
	unsigned x = width < mWidth ? (mWidth - width) / 2 : 0;
	unsigned y = mHeight / 30;

	for (const char *c = logo; *c; ++c) {
		bp_glyph_s (x, y, *c, 0xFFFFFFFFu, 0xFF000000u, scale);
		x += 6 * scale;
	}
}

static void
bp_header (void)
{
	uint64_t ns = 0;
	unsigned seconds = 0;
	unsigned saved_x = mX, saved_y = mY;
	static const char spin[] = "|/-\\";

	if (mStart) {
		absolutetime_to_nanoseconds (mach_absolute_time () - mStart, &ns);
		seconds = (unsigned)(ns / NSEC_PER_SEC);
	}
	bp_bar ();
	bp_fill (0, mHeaderY, mWidth, BP_LINE, 0xFF000000u);
	mX = 0;
	mY = mHeaderY;
	bp_text ("ALIVE ");
	bp_number (seconds, 10);
	bp_text ("s ");
	bp_emit (spin[mBeat & 3]);
	bp_text (" UFS ");
	bp_number (native_ufs_verified_blocks, 10);
	bp_emit ('/');
	bp_number (native_ufs_block_reads, 10);
	bp_text (" MD ");
	bp_number (marble_display_init, 10);
	bp_emit ('/');
	bp_number (marble_display_started, 10);
	bp_emit ('/');
	bp_number (marble_display_calls, 10);
	bp_text (" POL ");
	bp_number ((uint64_t)(unsigned)(sheng_launch_policy_status < 0 ? 9 : sheng_launch_policy_status), 10);
	bp_text (" CPU ");
	bp_number ((uint64_t)(unsigned)(processor_avail_count > 0 ? processor_avail_count : 0), 10);
	bp_emit ('/');
	bp_number ((uint64_t)(unsigned)(processor_count > 0 ? processor_count : 0), 10);
	bp_emit ('\n');
	mX = saved_x;
	mY = saved_y;
}

static uint64_t mLastBeat;

static void
bp_beat (void)
{
	uint64_t now = mach_absolute_time ();

	if (!mReady || (mLastBeat && now - mLastBeat < mBeatInterval) || !bp_lock ()) {
		return;
	}
	mLastBeat = now;
	mBeat++;
	bp_header ();
	{
		unsigned scale = 2, cell = 6 * scale, line = 14 * scale, y = mHeight * 7 / 100 + 20;
		bp_fill (0, y, mWidth, 3 * line, 0xFF000000u);
		unsigned col = 0;
		const char *head = "CORES step ";
		for (const char *c = head; *c; c++, col++) {
			bp_glyph_s (col * cell, y, *c, 0xFFFFFFFFu, 0xFF000000u, scale);
		}
		for (unsigned i = 0; i < 8; i++) {
			char digits[8]; unsigned n = 0; uint64_t v = armcpu_smp_step[i];
			do { digits[n++] = (char)('0' + v % 10); v /= 10; } while (v && n < sizeof (digits));
			while (n) { bp_glyph_s (col * cell, y, digits[--n], 0xFFFFFFFFu, 0xFF000000u, scale); col++; }
			bp_glyph_s (col * cell, y, i == 7 ? ' ' : ',', 0xFFFFFFFFu, 0xFF000000u, scale); col++;
		}
		col = 0;
		const char *head2 = "TIMER ";
		for (const char *c = head2; *c; c++, col++) {
			bp_glyph_s (col * cell, y + line, *c, 0xFFFFFFFFu, 0xFF000000u, scale);
		}
		for (unsigned i = 0; i < 8; i++) {
			char ch = armcpu_core_timer[i] ? '1' : '0';
			bp_glyph_s (col * cell, y + line, ch, 0xFFFFFFFFu, 0xFF000000u, scale); col++;
		}
		col = 0;
		const char *head3 = "FAIL ";
		for (const char *c = head3; *c; c++, col++) {
			bp_glyph_s (col * cell, y + 2 * line, *c, 0xFFFFFFFFu, 0xFF000000u, scale);
		}
		for (unsigned i = 0; i < 8; i++) {
			char digits[8]; unsigned n = 0; uint64_t v = armcpu_smp_fail[i];
			do { digits[n++] = (char)('0' + v % 10); v /= 10; } while (v && n < sizeof (digits));
			while (n) { bp_glyph_s (col * cell, y + 2 * line, digits[--n], 0xFFFFFFFFu, 0xFF000000u, scale); col++; }
			bp_glyph_s (col * cell, y + 2 * line, i == 7 ? ' ' : ',', 0xFFFFFFFFu, 0xFF000000u, scale); col++;
		}
	}
	bp_unlock ();
}

void
boot_progress_init (void)
{
}

void
boot_progress_bind (uintptr_t base, unsigned width, unsigned height, unsigned rowbytes, unsigned depth)
{
	static uintptr_t last_base;

	if (depth != 32 || !base || !width || !height || rowbytes < width * 4) {
		return;
	}
	if (base == last_base && mReady) {
		return;
	}
	last_base = base;
	mBase = (uint8_t *)base;
	mWidth = width;
	mHeight = height;
	mRowBytes = rowbytes;
	mCols = width / BP_CELL;
	mBarY = mHeight * 12 / 100;
	mStatusY = mBarY + 92;
	mHeaderY = mHeight * 20 / 100;
	mLogY = mHeaderY + BP_LINE;
	mX = 0;
	mY = mLogY;
	mReady = 1;
	if (bp_lock ()) {
		bp_fill (0, 0, mWidth, mHeight, 0xFF000000u);
		bp_splash ();
		bp_header ();
		bp_text_at (0, mStatusY, BP_SCALE, "EXEC -", 0xFFFFFFFFu);
		bp_text ("MARBLE XNU BOOT\n");
		bp_unlock ();
	}
}

int
boot_progress_active (void)
{
	/* Keep the serial console for kprintf: the diagnostic page only draws
	 * the explicit stage/exec lines and the UFS-driven heartbeat. */
	return 0;
}

void
boot_progress_putc (char c)
{
	if (!mReady || !bp_lock ()) {
		return;
	}
	bp_emit (c);
	bp_unlock ();
}

void
boot_progress_stage (const char *message)
{
	if (!mReady || !message || !bp_lock ()) {
		return;
	}
	bp_text ("> ");
	bp_text (message);
	bp_emit ('\n');
	bp_unlock ();
}

void
boot_progress_start (void)
{
	mStart = mach_absolute_time ();
	nanoseconds_to_absolutetime (NSEC_PER_SEC, &mBeatInterval);
	boot_progress_stage ("kernel bootstrap thread start");
}

void
boot_progress_process_stage (int pid, const char *name, const char *stage)
{
	static uint64_t window;
	static unsigned lines;
	uint64_t now = mach_absolute_time ();

	if (!mReady || !bp_lock ()) {
		return;
	}
	if (!window || now - window >= mBeatInterval) {
		window = now;
		lines = 0;
	}
	if (lines >= 12) {
		bp_unlock ();
		return;
	}
	lines++;
	{
		unsigned saved_x = mX, saved_y = mY;
		unsigned used = 0;
		bp_fill (0, mStatusY, mWidth, BP_LINE, 0xFF000000u);
		mX = 0;
		mY = mStatusY;
		const char *text[3] = { "EXEC ", name ? name : "?", stage ? stage : "?" };
		for (unsigned part = 0; part < 3; part++) {
			for (const char *c = text[part]; *c && used < 52; c++) {
				bp_emit (*c);
				used++;
			}
		}
		mX = saved_x;
		mY = saved_y;
	}
	bp_unlock ();
}

void
boot_progress_critical_exit (int pid, char *text, unsigned long size)
{
	(void)pid;
	(void)size;
	if (!mReady || !text || !bp_lock ()) {
		return;
	}
	bp_text ("CRITICAL ");
	bp_text (text);
	bp_emit ('\n');
	bp_unlock ();
}

void
boot_progress_exit (int pid, const char *name, int status, unsigned ns, uint64_t code)
{
	if (!mReady || ns == 0 || !bp_lock ()) {
		return;
	}
	bp_text ("EXIT ");
	bp_text (name ? name : "?");
	bp_emit ('[');
	bp_number ((uint64_t)(unsigned)pid, 10);
	bp_text ("] st ");
	bp_number ((uint64_t)(unsigned)status, 10);
	bp_text (" ns ");
	bp_number (ns, 10);
	bp_text (" code ");
	bp_number (code, 16);
	bp_emit ('\n');
	bp_unlock ();
}

void
boot_progress_user_breakpoint (void *state, unsigned esr)
{
	(void)state;
	__atomic_fetch_add (&mBrkCount, 1, __ATOMIC_RELAXED);
	mBrkEsr = esr;
}

void
boot_progress_user_abort (void *state, unsigned esr, uint64_t fault)
{
	(void)state;
	__atomic_fetch_add (&mAbortCount, 1, __ATOMIC_RELAXED);
	mAbortEsr = esr;
	mAbortFar = (unsigned)fault;
}

void
boot_progress_irq (uint64_t pc, uint64_t cpsr, uint64_t sp)
{
	(void)pc;
	(void)cpsr;
	(void)sp;
	__atomic_fetch_add (&mIrqCount, 1, __ATOMIC_RELAXED);
}

void
boot_progress_poll (void)
{
	bp_beat ();
}

