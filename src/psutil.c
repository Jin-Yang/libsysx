
#include "psutil.h"
#include "common.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <dirent.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <sys/sysinfo.h>

#define MAX_ARGS      1024
#define FILE_LEN      64

/* static array size */
#define SASIZE(a)     (sizeof(a) / sizeof(*(a)))

#define MOD "(psutil) "

#define _isspace0(c) ((c) == '\t' || (c) == '\n' || (c) == ' ' || (c) == '\0')
#define _isspace(c)  ((c) == '\t' || (c) == '\n' || (c) == ' ')
#define strim_head(beg) do {                                \
        while (_isspace((unsigned char)*beg))               \
                beg++;                                      \
} while(0)
#define strim_tail(beg, size) do {                          \
        char *end = beg + size;                             \
        while (end > beg && _isspace0((unsigned char)*end)) \
                end--;                                      \
        end[1] = 0;                                         \
} while(0)

#define ps_open_file(format, pid, ivd) do {                                   \
	rc = snprintf(file, sizeof(file), format, pid);                       \
	if (rc < 0 || rc >= (int)sizeof(file)) {                              \
		log_error(MOD "format file '" format "' failed, rc %d",       \
				pid, rc);                                     \
		return ivd;                                                   \
	}                                                                     \
                                                                              \
	fd = open(file, O_RDONLY);                                            \
	if (fd < 0) {                                                         \
		log_error(MOD "open(%s) failed, %s", file, strerror(errno));  \
		return ivd;                                                   \
	}                                                                     \
	                                                                      \
	rc = read(fd, buffer, sizeof(buffer));                                \
	if (rc == (int)sizeof(buffer)) {                                      \
		log_warning(MOD "read(%s) only got partial string", file);    \
		rc = sizeof(buffer) - 1;                                      \
	} else if (rc < 0) {                                                  \
		log_error(MOD "read(%s) failed, %s", file, strerror(errno));  \
		close(fd);                                                    \
		return ivd;                                                   \
	}                                                                     \
	buffer[rc] = 0;                                                       \
	close(fd);                                                            \
} while(0)

static long HERTZ = -1;
static long HERPAGESIZE = -1;

char *ps_get_comm(pid_t pid)
{
        int rc, fd;
        char file[FILE_LEN], buffer[MAX_ARGS];

        ps_open_file("/proc/%d/comm", pid, NULL);
        strim_tail(buffer, rc);

        return strdup(buffer);
}

static int strsplit(char *string, char **fields, int size)
{
	int i = 0;
	char *ptr = string, *saveptr = NULL;

	while ((fields[i] = strtok_r(ptr, ", \t\r\n", &saveptr)) != NULL) {
		ptr = NULL;
		i++;

		if (i >= size)
			break;
	}

	return i;
}

char *ps_get_cmdline(pid_t pid)
{
	int rc, fd, i;
	char file[FILE_LEN], buffer[MAX_ARGS];

	ps_open_file("/proc/%d/cmdline", pid, NULL);

	for (i = 0; i < rc; i++) {
		if (buffer[i] == '\0')
			buffer[i] = ' ';
	}
	buffer[--i] = '\0';

	return strdup(buffer);
}

int ps_get_fd_count(int pid)
{
        DIR *dh;
        char dirname[64]; /* max /proc/65536/fd */
        struct dirent *ent;
        int count = 0, rc;

        rc = snprintf(dirname, sizeof(dirname), "/proc/%i/fd", pid);
        if (rc < 0 || rc > (int)sizeof(dirname)) {
                log_error(MOD "failed to get fd count, format directory failed, rc %d.", rc);
                return -1;
        }

        dh = opendir(dirname);
        if (dh == NULL) {
                log_error(MOD "failed to open directory '%s', %s.", dirname, strerror(errno));
                return -1;
        }
        while ((ent = readdir(dh)) != NULL) {
                if (ent->d_name[0] < '0' || ent->d_name[0] > '9')
                        continue;
                count++;
        }
        closedir(dh);

        return count;
}

long ps_get_uptime(void)
{
	int rc, fd;
	struct sysinfo info;
	char buffer[64];

	rc = sysinfo(&info);
	if (rc == 0)
		return info.uptime;

	log_warning(MOD "call sysinfo failed, %s", strerror(errno));

	fd = open("/proc/uptime", O_RDONLY);
	if (fd < 0) {
		log_error(MOD "open('/proc/uptime') failed, %s", strerror(errno));
		return -1;
	}

	rc = read(fd, buffer, sizeof(buffer) - 1);
	if (rc <= 0) {
		log_error(MOD "read('/proc/uptime') failed, %s", strerror(errno));
		close(fd);
		return -1;
	}

	buffer[rc] = 0;
	close(fd);

	return atoll(buffer);
}

int ps_init(void)
{
	long data;

	data = sysconf(_SC_CLK_TCK);
	if (data < 0) {
		log_error(MOD "get HZ failed, %s", strerror(errno));
		return -1;
	}
	HERTZ = data;

	data = sysconf(_SC_PAGESIZE);
	if (data < 0) {
		log_error(MOD "get PAGESIZE failed, %s", strerror(errno));
		return -1;
	}
	HERPAGESIZE = data;

	return 0;
}

/*
 * NOTE:
 *    1. CPU Usage.
 *       if info._.uptime <= 0, get the average cpusage since process started.
 *          watch -n 1 "ps -o ruser,pid,%cpu,%mem,vsz,rss,comm p `pidof foobar`"
 *       else info._.uptime <= 0, get the average cpusage since last update.
 *          top -d 1 -p `pidof foobar`
 */
int ps_get_process(pid_t pid, struct procinfo *info)
{
        int rc;
        long protime, uptime, ttime;
        char *fields[32], buff[4096];

        if (pid <= 0 || info == NULL) {
                log_error(MOD "invalid arguments (pid, info) = (%d, %p).",
                                pid, info);
                return -ERR_COMMON;
        }

        rc = ps_read_file(buff, sizeof(buff), "/proc/%d/stat", pid);
        if (rc < 0) /* error message already print in the function */
                return -ERR_COMMON;
        rc = strsplit(buff, fields, SASIZE(fields));
        if (rc < 25) {
                log_error(MOD "only %d fields got from '/proc/%d/stat' file, expect >= 25",
                                rc, pid);
                return -1;
        }

        uptime = ps_get_uptime();
        if (uptime < 0)
                return -1;

        info->rss = atoll(fields[23]) * HERPAGESIZE / 1024; /* rss (24), KBytes */

        /* [(utime + stime) - last(utime + stime)] / {[bootime - last(bootime)] * HZ} */
        protime = atoll(fields[13]) + atoll(fields[14]); /* utime (14) + stime (15) */
        if (info->_.uptime <= 0)
                ttime = (uptime * HERTZ - atoll(fields[21])); /* starttime (22) */
        else
                ttime = (uptime - info->_.uptime) * HERTZ;
        if (ttime <= 0) {
                log_error("got invalid boot time %ld, uptime %ld last %ld.",
                                ttime, uptime, info->_.uptime);
                return -1;
        }
        info->cpu_usage = ((double)(protime - info->_.protime) / (double)ttime) * 100.;

        info->_.uptime = uptime;
        info->_.protime = protime;

        return 0;
}
#undef ps_open_file

