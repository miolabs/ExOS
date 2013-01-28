#include <kernel/machine/time_hal.h>
#include <support/lpc2k/timer.h>

#ifndef TICK_TIMER
#define TICK_TIMER 0
#endif

void hal_time_initialize(int period_us)
{
	// setup tick (1ms) timer
	timer_initialize(TICK_TIMER, 1000000, period_us - 1, TIMER_MODE_CONTINUOUS_RELOAD, 
		(MATCH_HANDLER)__kernel_tick, 0);
}
