#ifndef dbug_hh
#define dbug_hh

#include <time.h>
#include <stdio.h>

#define DUMP_TIME(s) {						\
    timespec ttv; clock_gettime(CLOCK_REALTIME,&ttv);		\
    printf("DUMP_TIME[%s:%d] %d.%09d [%s]\n",__FILE__,__LINE__, \
	   ttv.tv_sec,ttv.tv_nsec,s); }

#endif
