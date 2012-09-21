// ExOS posix layer
// reference:
// http://pubs.opengroup.org/onlinepubs/7908799/xsh/time.h.html

#ifndef __posix_time_h
#define __posix_time_h

#include <sys/types.h>

#include <kernel/timer.h>

struct tm
{
	int tm_sec;   //seconds [0,61]
	int tm_min;   // minutes [0,59]
	int tm_hour;  // hour [0,23]
	int tm_mday;  // day of month [1,31]
	int tm_mon;   // month of year [0,11]
	int tm_year;  // years since 1900
	int tm_wday;  // day of week [0,6] (Sunday = 0)
	int tm_yday;  // day of year [0,365]
	int tm_isdst; // daylight savings flag
};

#ifndef NULL
#define NULL (void *)
#endif

#define CLOCKS_PER_SEC (EXOS_TICK_FREQ)

struct timespec
{
	time_t tv_sec;	// seconds
	long tv_nsec;	// nanoseconds
};


#endif // __posix_time_h


