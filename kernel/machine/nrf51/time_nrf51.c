#include <kernel/machine/time_hal.h>
#include <support/pwm_hal.h>
#include <CMSIS/nrf51.h>

#ifndef TICK_TIMER
#define TICK_TIMER 2
#endif

static void _callback(int module)
{
	__kernel_tick();
}

void hal_time_initialize(int period_us)
{
	hal_pwm_initialize(TICK_TIMER, 1000, 1000, 3);
	hal_pwm_set_handler(TICK_TIMER, 3, _callback);
}

