#include <kernel/machine/time_hal.h>
#include <support/lpc2k/timer.h>

#ifndef TICK_TIMER
#define TICK_TIMER 0
#endif

void hal_time_initialize(int period_us)
{
	// setup tick (1ms) timer
	timer_match_initialize(TICK_TIMER, 1000, 1000000 / period_us, 3);
	timer_match_set_handler(TICK_TIMER, 3, (HAL_PWM_HANDLER)__kernel_tick);
}
