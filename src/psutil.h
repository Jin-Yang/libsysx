#ifndef BOOTER_PSUTIL_H_
#define BOOTER_PSUTIL_H_

#include <fcntl.h>

#define ERR_COMMON 1
#define ERR_FORMAT 2

struct procinfo {
	struct {
		long uptime;
		long protime;
	} _;
	long rss; /* Kbytes */
	double cpu_usage; /* 0.9 <-> 90% */
};
#define PROCINFO_INIT   { ._.uptime = 0, ._.protime = 0, .rss = 0, .cpu_usage = 0. }

int ps_init(void);
char *ps_get_comm(pid_t pid);
int ps_get_fd_count(int pid);
int ps_get_process(pid_t pid, struct procinfo *info);



struct cgrpinfo {
        long cache;
        long rss;
        long rss_huge;
        long mapped_file;
        long swap;
};

int ps_cgrp_stats(const char *path, struct cgrpinfo *info);

#endif
