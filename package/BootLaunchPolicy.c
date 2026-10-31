// SPDX-License-Identifier: MIT
// Bring-up launch policy for the scratch Data volume: disable the launchd
// jobs that cannot work on this port yet (no FairPlay, no display service)
// through /private/var/db/com.apple.xpc.launchd/disabled.plist before the
// unchanged launchd exec. Best effort: a failure is printed, never fatal.
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/sysproto.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <pexpert/pexpert.h>

static const char policy_head[] =
 "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
 "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
 "<plist version=\"1.0\"><dict>";
static const char policy_tail[] = "</dict></plist>\n";
static char policy[2048], jobs[200];
volatile int sheng_launch_policy_status = -1;

static int
append_job (const char **cursor)
{
	const char *c = *cursor, *e = c;

	while (*e && *e != ',') {
		if (!((*e >= 'a' && *e <= 'z') || (*e >= 'A' && *e <= 'Z') ||
		    (*e >= '0' && *e <= '9') || *e == '.' || *e == '-')) {
			return EINVAL;
		}
		++e;
	}
	if (e > c) {
		size_t at = strlen (policy);
		if (at + (size_t)(e - c) + 16 >= sizeof (policy)) {
			return EINVAL;
		}
		memcpy (policy + at, "<key>", 5);
		at += 5;
		memcpy (policy + at, c, (size_t)(e - c));
		at += (size_t)(e - c);
		memcpy (policy + at, "</key><true/>", 13);
		policy[at + 13] = 0;
	}
	*cursor = *e ? e + 1 : e;
	return 0;
}

int
sheng_boot_watchdog_policy (void *process, uint64_t scratch)
{
	proc_t  p = process;
	int     watchdog = 0, error = 0, fd = -1;
	int32_t result = 0;

	PE_parse_boot_argn ("sheng-disable-watchdogd", &watchdog, sizeof (watchdog));
	if (!PE_parse_boot_argn ("sheng-disable-jobs", jobs, sizeof (jobs))) {
		jobs[0] = 0;
	}
	jobs[sizeof (jobs) - 1] = 0;
	if (watchdog != 1) {
		watchdog = 0;
	}
	if (!watchdog && !jobs[0]) {
		sheng_launch_policy_status = 0;
		return 0;
	}
	policy[0] = 0;
	strlcat (policy, policy_head, sizeof (policy));
	if (watchdog) {
		strlcat (policy, "<key>com.apple.watchdogd</key><true/>", sizeof (policy));
	}
	for (const char *c = jobs; *c;) {
		if ((error = append_job (&c))) {
			goto done;
		}
	}
	strlcat (policy, policy_tail, sizeof (policy));
	const char *directory = "/private/var/db/com.apple.xpc.launchd";
	const char *path = "/private/var/db/com.apple.xpc.launchd/disabled.plist";
	if ((error = copyout (directory, scratch, strlen (directory) + 1))) {
		goto done;
	}
	struct mkdir_args md = { .path = scratch, .mode = 0755 };
	error = mkdir (p, &md, &result);
	if (error && error != EEXIST) {
		goto done;
	}
	if ((error = copyout (path, scratch, strlen (path) + 1))) {
		goto done;
	}
	struct open_args op = { .path = scratch, .flags = O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW, .mode = 0644 };
	if ((error = open (p, &op, &result))) {
		goto done;
	}
	fd = result;
	/* The cache-mount environment slots live at +0x400, +0x500, +0x600. */
	for (size_t sent = 0, total = strlen (policy); sent < total;) {
		size_t piece = total - sent < 0x1f0 ? total - sent : 0x1f0;
		if ((error = copyout (policy + sent, scratch + 0x200, piece))) {
			goto done;
		}
		struct write_args wr = { .fd = fd, .cbuf = scratch + 0x200, .nbyte = piece };
		user_ssize_t written = 0;
		if ((error = write (p, &wr, &written))) {
			goto done;
		}
		if (written != (user_ssize_t)piece) {
			error = EIO;
			goto done;
		}
		sent += piece;
	}
	struct fchown_args owner = { .fd = fd, .uid = 0, .gid = 0 };
	if ((error = fchown (p, &owner, &result))) {
		goto done;
	}
	struct fchmod_args mode = { .fd = fd, .mode = 0644 };
	if ((error = fchmod (p, &mode, &result))) {
		goto done;
	}
	struct fsync_args sync = { .fd = fd };
	if ((error = fsync (p, &sync, &result))) {
		goto done;
	}
	struct fstat64_args sa = { .fd = fd, .ub = scratch + 0x800 };
	struct stat64 st = {0};
	if ((error = sys_fstat64 (p, &sa, &result)) || (error = copyin (sa.ub, &st, sizeof (st)))) {
		goto done;
	}
	if (st.st_uid != 0 || st.st_gid != 0 || !S_ISREG (st.st_mode) ||
	    (st.st_mode & 0777) != 0644 || st.st_size != (off_t)strlen (policy)) {
		error = EINVAL;
		goto done;
	}
done:
	if (fd >= 0) {
		struct close_args cl = { .fd = fd };
		int close_error = sys_close (p, &cl, &result);
		if (!error) {
			error = close_error;
		}
	}
	if (!error) {
		const char *marker = "/private/var/db/com.apple.xpc.launchd/disabled.migrated";
		error = copyout (marker, scratch, strlen (marker) + 1);
		if (!error) {
			struct open_args mark = { .path = scratch, .flags = O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, .mode = 0644 };
			error = open (p, &mark, &result);
			if (!error) {
				struct close_args cl = { .fd = result };
				error = sys_close (p, &cl, &result);
			} else if (error == EEXIST) {
				error = 0;
			}
		}
	}
	printf ("sheng bring-up launch policy: %d (%s)\n", error, error ? "not applied" : "disabled.plist written");
	sheng_launch_policy_status = error;
	/* The policy is advisory: never stop the boot for it. */
	return 0;
}
