#include "unistd.h"
#include <kernel/thread.h>
#include <kernel/timer.h>

unsigned sleep(unsigned seconds)
{
	exos_thread_sleep(seconds * EXOS_TICK_FREQ);
	return 0;
	// NOTE: should return remaining time 
	// but that case should not happen in ExOS
}
