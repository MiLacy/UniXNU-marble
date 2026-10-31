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
		uint32_t *line = (uint32_t *)(mBase + (size_t)(y + row) * mRowBytes);
		for (unsigned col = 0; col < w && x + col < mWidth; col++) {
			line[x + col] = color;
		}
	}
}

static void
bp_glyph (unsigned x, unsigned y, char c, uint32_t fg, uint32_t bg)
{
	uint64_t glyph;
	unsigned top, bottom;

	if (c < 32 || c > 126) {
		c = '?';
	}
	glyph = mFont[c - 32];
	top = (unsigned)(glyph >> 32);
	bottom = (unsigned)glyph;
	for (unsigned row = 0; row < 14 * BP_SCALE; row++) {
		uint32_t *line = (uint32_t *)(mBase + (size_t)(y + row) * mRowBytes);
		for (unsigned col = 0; col < BP_CELL; col++) {
			line[x + col] = bg;
		}
	}
	for (unsigned row = 0; row < 12; row++) {
		unsigned data = (row < 6) ? (top >> (row * 5)) : (bottom >> ((row - 6) * 5));
		for (unsigned col = 0; col < 5; col++) {
			if ((data >> col) & 1) {
				for (unsigned dy = 0; dy < BP_SCALE; dy++) {
					uint32_t *line = (uint32_t *)(mBase + (size_t)(y + row * BP_SCALE + dy) * mRowBytes);
					for (unsigned dx = 0; dx < BP_SCALE; dx++) {
						line[x + col * BP_SCALE + dx] = fg;
					}
				}
			}
		}
	}
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
		size_t bytes = (size_t)(mHeight - 2 * BP_LINE) * mRowBytes;
		memmove ((void *)mBase + BP_LINE * mRowBytes,
		    (const void *)mBase + 2 * BP_LINE * mRowBytes, bytes);
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
	bp_fill (0, 0, mWidth, BP_LINE, 0xFF000000u);
	mX = 0;
	mY = 0;
	bp_text ("ALIVE ");
	bp_number (seconds, 10);
	bp_text ("s ");
	bp_emit (spin[mBeat & 3]);
	bp_text (" UFS ");
	bp_number (native_ufs_verified_blocks, 10);
	bp_emit ('/');
	bp_number (native_ufs_block_reads, 10);
	bp_text (" IRQ ");
	bp_number (mIrqCount, 10);
	bp_text (" BRK ");
	bp_number (mBrkCount, 10);
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
	mX = 0;
	mY = 0;
	mReady = 1;
	if (bp_lock ()) {
		bp_fill (0, 0, mWidth, mHeight, 0xFF000000u);
		bp_header ();
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
	bp_text (name ? name : "?");
	bp_emit ('[');
	bp_number ((uint64_t)(unsigned)pid, 10);
	bp_text ("] ");
	bp_text (stage ? stage : "?");
	bp_emit ('\n');
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

