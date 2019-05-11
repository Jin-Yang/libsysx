#include "../src/psutil.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
        int i, count = 1, pid;
        struct procinfo info = PROCINFO_INIT;

        if (argc < 2) {
                fprintf(stderr, "Usage: %s <PID> [count]\n", argv[0]);
                exit(1);
        }
        pid = atoi(argv[1]);
        if (argc > 2)
                count = atoi(argv[2]);

        if (ps_init() < 0) {
                fprintf(stderr, "Init psutil failed\n");
                exit(1);
        }

        for (i = 0; i < count; i++) {
                if (ps_get_process(pid, &info) < 0)
                        break;
                printf("PID %d current resource, CPU %.2f%% MEM %ldKB\n",
                                pid, info.cpu_usage, info.rss);
                sleep(1);
        }

        return 0;
}
