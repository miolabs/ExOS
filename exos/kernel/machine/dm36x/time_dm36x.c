#include <kernel/machine/time_hal.h>
#include <support/dm36x/timer.h>

#define TICK_TIMER 0

static TIMER_MODULE *_mod_tick;

void hal_time_initialize(int period_us)
{
	// setup tick (1ms) timer
	_mod_tick = timer_initialize(TICK_TIMER, 1000000, period_us, TIMER_MODE_CONTINUOUS);
   	timer_set_handler(TICK_TIMER, (MATCH_FN)__kernel_tick, INTC_PRI_IRQ_LOWEST);
	timer_control(TICK_TIMER, 1);
}
