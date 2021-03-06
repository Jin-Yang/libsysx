
#include "psutil.h"
#include "common.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

#include <fcntl.h>

#define MOD "(psutil) "

#define CGM_STATS_FILE "memory.stat"


int ps_get_from_file(const char *path, uint64_t *ret)
{
        /* MAX(uint64_t): 18446744073709551615 */
        int fd, rc;
        char buffer[32];

        if (path == NULL || ret == NULL)
                return -1;

        fd = open(path, O_RDONLY);
        if (fd < 0)
                return -1;
        rc = read(fd, buffer, sizeof(buffer));
        if (rc < 0) {
                close(fd);
                return -1;
        }
        buffer[rc] = 0;
        close(fd);

        errno = 0;
        *ret = strtoull(buffer, NULL, 10);
        if (errno == ERANGE)
                return -1;

        return 0;
}




int ps_cgrp_stats(const char *path, struct cgrpinfo *info)
{
        int rc, fd, i, offset;
        char buffer[256], *ptr;

        rc = snprintf(buffer, sizeof(buffer), "%s/" CGM_STATS_FILE, path);
        if (rc < 0 || rc >= (int)sizeof(buffer)) {
                log_error(MOD "format file path '%s' failed, rc %d.", path, rc);
                return -1;
        }

        fd = open(buffer, O_RDONLY);
        if (fd < 0) {
                log_error(MOD "open('%s/" CGM_STATS_FILE "') failed, %s.",
                                path, strerror(errno));
                return -1;
        }

        offset = 0;
        while (1) {
                rc = read(fd, buffer + offset, sizeof(buffer) - offset);
                if (rc < 0) {
                        log_error(MOD "read ('%s/" CGM_STATS_FILE "') failed, %s.",
                                        path, strerror(errno));
                        close(fd);
                        return -1;
                } else if (rc == 0) {
                        break;
                }
                //log_debug(MOD "Got %dB %s", rc, buffer);

                rc += offset;
                ptr = buffer;
                for (i = 0; i < rc; i++) {
                        if (buffer[i] == '\n') {
                                buffer[i] = 0;

                                if (strncmp(ptr, "cache ", 6) == 0) {
                                        info->cache = atol(ptr + 6);
                                } else if (strncmp(ptr, "rss ", 4) == 0) {
                                        info->rss = atol(ptr + 4);
                                } else if (strncmp(ptr, "rss_huge ", 9) == 0) {
                                        info->rss_huge = atol(ptr + 9);
                                } else if (strncmp(ptr, "mapped_file ", 12) == 0) {
                                        info->mapped_file = atol(ptr + 12);
                                } else if (strncmp(ptr, "swap ", 5) == 0) {
                                        info->swap = atol(ptr + 5);
                                }
                                ptr = buffer + i + 1;
                        }
                }
                //log_debug(MOD "===> END buffer(%p) ptr(%p) %s", buffer + rc, ptr, ptr);
                if (ptr < buffer + rc) {
                        rc = buffer + rc - ptr;
                        //log_debug(MOD "===> buffer(%p) ptr(%p) rc(%d)", buffer, ptr, rc);
                        memmove(buffer, ptr, rc);
                        offset = rc;
                }
        }
        close(fd);

        return 0;
}
#if 0
static int psc_cpusage(const char *path, gauge_t *ret)
{
        static unsigned long long usage;
        static cdtime_t last = -1;
        unsigned long long curr;
        cdtime_t now = cdtime();
        int rc;
        char file[PATH_MAX];

        assert(sizeof(unsigned long long) == sizeof(uint64_t));
        snprintf_s(file, sizeof(file), sizeof(file), "%s/cpuacct.usage", path);

        rc = get_int_from_file(file, (uint64_t *)&curr);
        if (rc < 0) {
                char errbuf[1024];
                ERROR(PLUGIN_NAME "Read from '%s' failed, rc %d, %s.", file, rc,
                        sstrerror(errno, errbuf, sizeof(errbuf)));
                return -1;
        }

        double diff = CDTIME_T_TO_DOUBLE(now) - CDTIME_T_TO_DOUBLE(last);
        if (last <= 0 || curr < usage || diff < 0) {
                last = now;
                usage = curr;
                *ret = 0;
                return 0;
        }
        *ret = (curr - usage) / (diff) / 1e7;
        last = now;
        usage = curr;

        return 0;
}
#endif
