#include "systick.h"
#include <kernel/machine/time_hal.h>
#include <kernel/panic.h>

void hal_time_initialize(int period_us)
{
	// TODO: configure according to requested period

	SysTick->VAL = 0; 
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
		SysTick_CTRL_TICKINT_Msk |
		SysTick_CTRL_ENABLE_Msk;
}

void SysTick_Handler()
{
#ifdef EXOS_OLD_TICK_API
	__kernel_tick();
#else
#error "ULP API not implemented"
#endif
}

