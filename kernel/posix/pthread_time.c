#include <pthread.h>
#include <errno.h>

#include <kernel/thread.h>

static unsigned long _ticks(const struct timespec *ts)
{
	unsigned long ticks = ts->tv_nsec / (EXOS_TICK_MICROS * 1000);
	ticks += ts->tv_sec * EXOS_TICK_FREQ;
	return ticks;
}

int nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
	unsigned long ticks = _ticks(rqtp);
	exos_thread_sleep(ticks);

	// return remaining time, always zero
	*rmtp = (struct timespec) { .tv_sec = 0, .tv_nsec = 0 };
	return 0;
}



