#include <kernel/machine/time_hal.h>
#include <CMSIS/LPC177x_8x.h>
#include <CMSIS/system_LPC177x_8x.h> 

void hal_time_initialize(int period_us)
{
	SysTick_Config((long long)SystemCoreClock * period_us / 1000000);
}

void SysTick_Handler() 
{
	__kernel_tick();
}
