#ifndef __posix_sys_time_h
#define __posix_sys_time_h

#include <sys/types.h>

struct timeval
{
	time_t tv_sec;
	suseconds_t	tv_usec;
};

#endif // __posix_sys_time_h


