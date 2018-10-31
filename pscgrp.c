
#include "psutil.h"
#include "common.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <fcntl.h>

#define MOD "(psutil) "

#define CGM_STATS_FILE "memory.stat"

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

