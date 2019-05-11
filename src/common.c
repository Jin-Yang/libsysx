#include "common.h"
#include "psutil.h"

#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MOD ""

/* 
 * 123456789 123456789 123456789 123456789
 * /proc/65536/net/ip_tables_targets 
 */
#define PROC_FILE_LEN_MAX      64

/* read size bytes from file, maybe only partial of the file. */
int ps_read_file(char *buff, int size, const char *fmt, ...)
{
        va_list ap;
        int rc, fd;
        char path[PROC_FILE_LEN_MAX];

        va_start(ap, fmt);
        rc = vsnprintf(path, sizeof(path), fmt, ap);
        va_end(ap);
        if (rc < 0 || rc >= (int)sizeof(path)) {
                log_error(MOD "format file '%s' failed, rc %d.", fmt, rc);
                return -ERR_FORMAT;
        }
        buff[rc] = 0;

        fd = open(path, O_RDONLY);
        if (fd < 0) {
                log_error(MOD "open(%s) failed, %s.", path, strerror(errno));
                return -ERR_COMMON;
        }
        rc = read(fd, buff, size - 1);
        if (rc < 0) {
                log_error(MOD "read(%s) failed, %s.", path, strerror(errno));
                close(fd);
                return -ERR_COMMON;
        }
        buff[rc] = 0;
        close(fd);

        return rc;
}

int ps_read_int_from_file(const char *path, uint64_t *ret)
{
        /* MAX(uint64_t): 18446744073709551615 */
        int fd, rc;
        char buff[32], *ptr;

        if (path == NULL || ret == NULL)
                return -1;

        fd = open(path, O_RDONLY);
        if (fd < 0)
                return -1;
        rc = read(fd, buff, sizeof(buff) - 1);
        if (rc < 0) {
                close(fd);
                return -1;
        }
        buff[rc] = 0;
        close(fd);

        errno = 0;
        *ret = strtoull(buff, &ptr, 0);
        if (buff == ptr || errno == ERANGE)
                return -1;

        return 0;
}
