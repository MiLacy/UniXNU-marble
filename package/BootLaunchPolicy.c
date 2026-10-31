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
#include <sys/unistd.h>
#include <pexpert/pexpert.h>

extern proc_t initproc;

static const char policy_head[] =
 "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
 "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
 "<plist version=\"1.0\"><dict>";
static const char policy_tail[] = "</dict></plist>\n";
static char policy[2048], jobs[200];
volatile int sheng_launch_policy_status = -1;
uint64_t sheng_scratch;
volatile int sheng_crash_log_ready = -1;
char sheng_crash_log_text[1200];

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

	sheng_scratch = scratch;
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

struct sweet_dirent {
	uint64_t d_ino;
	uint16_t d_reclen;
	uint8_t  d_type;
	uint8_t  d_namlen;
	char     d_name[256];
};

static int
sweet_read_file (proc_t p, const char *path, uint64_t scratch, char *out, size_t want)
{
	int fd = -1, error = 0;
	int32_t result = 0;

	if ((error = copyout (path, scratch, strlen (path) + 1))) {
		return error;
	}
	struct open_args op = { .path = scratch, .flags = O_RDONLY, .mode = 0 };
	if ((error = open (p, &op, &result))) {
		return error;
	}
	fd = result;
	struct lseek_args ls = { .fd = fd, .offset = -(off_t)want, .whence = SEEK_END };
	off_t moved = 0;
	if (lseek (p, &ls, &moved)) {
		struct lseek_args zero = { .fd = fd, .offset = 0, .whence = 0 };
		(void)lseek (p, &zero, &moved);
	}
	struct read_args rd = { .fd = fd, .cbuf = scratch + 0x200, .nbyte = want - 1 };
	user_ssize_t got = 0;
	error = read (p, &rd, &got);
	if (!error) {
		if (got < 0 || got > (user_ssize_t)(want - 1)) {
			error = EIO;
		} else if ((error = copyin (scratch + 0x200, out, (size_t)got))) {
			;
		} else {
			out[got] = 0;
		}
	}
	struct close_args cl = { .fd = fd };
	int close_error = sys_close (p, &cl, &result);
	if (!error) {
		error = close_error;
	}
	return error;
}

static int
sweet_find_crash (proc_t p, uint64_t scratch, char *name, size_t namesz)
{
	static const char *directories[] = {
		"/private/var/log/CrashReporter",
		"/private/var/mobile/Library/Logs/CrashReporter",
	};
	char best[128] = "";

	for (unsigned d = 0; d < sizeof (directories) / sizeof (directories[0]); d++) {
		int fd = -1, error = 0;
		int32_t result = 0;
		if ((error = copyout (directories[d], scratch, strlen (directories[d]) + 1))) {
			continue;
		}
		struct open_args op = { .path = scratch, .flags = O_RDONLY | O_DIRECTORY, .mode = 0 };
		if ((error = open (p, &op, &result))) {
			continue;
		}
		fd = result;
		off_t position = 0;
		for (;;) {
			if (copyout (&position, scratch + 0x100, sizeof (position))) {
				break;
			}
			struct getdirentries64_args gd = { .fd = fd, .buf = scratch + 0x200,
			    .bufsize = 0x1000, .position = scratch + 0x100 };
			user_ssize_t got = 0;
			if (getdirentries64 (p, &gd, &got) || got <= 0) {
				break;
			}
			char block[0x1000];
			if (copyin (scratch + 0x200, block, (size_t)got) || copyin (scratch + 0x100, &position, sizeof (position))) {
				break;
			}
			for (off_t at = 0; at + 12 <= (off_t)got;) {
				struct sweet_dirent *entry = (struct sweet_dirent *)(block + at);
				uint16_t reclen = entry->d_reclen;
				uint8_t namlen = entry->d_namlen;
				if (!reclen || at + reclen > (off_t)got) {
					break;
				}
				if (namlen > 0 && namlen < sizeof (entry->d_name)) {
					char entry_name[128];
					size_t n = namlen < sizeof (entry_name) - 1 ? namlen : sizeof (entry_name) - 1;
					memcpy (entry_name, entry->d_name, n);
					entry_name[n] = 0;
					size_t len = strlen (entry_name);
					int match = (strncmp (entry_name, "backboardd", 10) == 0) ||
					    (strncmp (entry_name, "SpringBoard", 11) == 0);
					if (match && len > 4 && strcmp (entry_name + len - 4, ".ips") == 0 &&
					    strcmp (entry_name, best) > 0) {
						strlcpy (best, entry_name, sizeof (best));
					}
				}
				at += reclen;
			}
		}
		struct close_args cl = { .fd = fd };
		(void)sys_close (p, &cl, &result);
		if (best[0]) {
			strlcpy (name, best, namesz);
			return 0;
		}
	}
	return ENOENT;
}

int
sheng_crash_log_read (void)
{
	static char report[8192];
	char path[160], name[128];
	int error;

	proc_t p = initproc;
	if (!p || !sheng_scratch) {
		sheng_crash_log_ready = -1;
		return EAGAIN;
	}
	if ((error = sweet_find_crash (p, sheng_scratch, name, sizeof (name)))) {
		/* No crash report yet: show the launchd log tail instead. */
		if ((error = sweet_read_file (p, "/private/var/log/com.apple.xpc.launchd/launchd.log",
		    sheng_scratch, report, sizeof (report))) == 0) {
			size_t out = 0;
			const char *c = report;
			for (; *c && out < sizeof (sheng_crash_log_text) - 1; c++) {
				sheng_crash_log_text[out++] = (*c >= 32 && *c <= 126) ? *c : ' ';
			}
			sheng_crash_log_text[out] = 0;
			sheng_crash_log_ready = 0;
			printf ("sheng launchd log tail shown\n");
			return 0;
		}
		sheng_crash_log_ready = -1;
		return error;
	}
	snprintf (path, sizeof (path), "/private/var/log/CrashReporter/%s", name);
	if (sweet_read_file (p, path, sheng_scratch, report, sizeof (report))) {
		snprintf (path, sizeof (path), "/private/var/mobile/Library/Logs/CrashReporter/%s", name);
		if ((error = sweet_read_file (p, path, sheng_scratch, report, sizeof (report)))) {
			sheng_crash_log_ready = -1;
			return error;
		}
	}
	/* Keep the assertion text or the first exception line. */
	const char *pick = NULL;
	const char *keys[] = { "\"asi\"", "Assertion", "\"exception\"" };
	for (unsigned i = 0; i < 3 && pick == NULL; i++) {
		pick = strnstr (report, keys[i], sizeof (report));
	}
	if (pick == NULL) {
		pick = report;
	}
	size_t out = 0;
	for (const char *c = pick; *c && out < sizeof (sheng_crash_log_text) - 1; c++) {
		if (out >= 180) {
			break;
		}
		sheng_crash_log_text[out++] = (*c >= 32 && *c <= 126) ? *c : ' ';
	}
	sheng_crash_log_text[out] = 0;
	sheng_crash_log_ready = 0;
	printf ("sheng crash log: %s\n", name);
	return 0;
}
