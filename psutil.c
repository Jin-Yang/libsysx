
#include "psutil.h"
#include "common.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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

static int ps_get_fd_count(int pid)
{
        char dirname[64]; /* max /proc/65536/fd */
        DIR *dh;
        struct dirent *ent;
        int count = 0, rc;

        rc = snprintf(dirname, sizeof(dirname), "/proc/%i/fd", pid);
        if (rc < 0 || rc > (int)sizeof(dirname)) {
                log_error("failed to get fd count, format directory failed, rc %d.", rc);
                return -1;
        }

        dh = opendir(dirname);
        if (dh == NULL) {
                log_error("failed to open directory '%s', %s.", dirname, strerror(errno));
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

int ps_get_process(pid_t pid, struct procinfo *info)
{
	char file[FILE_LEN], buffer[4096];
	int rc, fd;
	char *fields[32];

	long utime, stime, starttime, boottime;

	ps_open_file("/proc/%d/stat", pid, -1);

	rc = strsplit(buffer, fields, SASIZE(fields));
	if (rc < 25) {
		log_error(MOD "only %d fields got for '%s', expect >= 25",
				rc, file);
		return -1;
	}

	boottime = ps_get_uptime();
	utime = atoll(fields[13]);     /* utime (14) */
	stime = atoll(fields[14]);     /* stime (15) */
	starttime = atoll(fields[21]); /* starttime (22) */

	info->rss = atoll(fields[23]) * HERPAGESIZE; /* rss (24), Bytes */
	info->cpu_usage = (double)(utime + stime) / (double)(boottime * HERTZ - starttime);

	return 0;
}

#undef ps_open_file

