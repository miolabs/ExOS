#include <kernel/machine/time_hal.h>
#include <CMSIS/nrf51.h>

void hal_time_initialize(int period_us)
{
	SysTick_Config((long long)SystemCoreClock * period_us / 1000000);
}

void SysTick_Handler() 
{
	__kernel_tick();
}
