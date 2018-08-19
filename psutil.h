#ifndef BOOTER_PSUTIL_H_
#define BOOTER_PSUTIL_H_

struct procinfo {
	long rss; /* bytes */
	double cpu_usage; /* 0.9 <-> 90% */
};

#endif
