#include <kernel/machine/hal.h>
#include <CMSIS/LPC177x_8x.h>
#include <support/board_hal.h>

void __machine_init()
{
	LPC_SC->PCLKSEL = LPC_SC->CCLKSEL & 0x1F;	// PCLK = CCLK
	//NOTE: CMSIS PeripheralClock value is outdated and may be incorrect
	// use legacy lpc17 family cpu_pclk() function

	hal_board_initialize();

	// set lowest priority for PendSV
	NVIC_SetPriority(PendSV_IRQn, 0xFF);	

	for (int i = 0; i <= 40; i++)
	{
		// set lowest priority for IRQ
		NVIC_SetPriority((IRQn_Type)i, 0xFF);
	}
}

void __machine_req_switch()
{
	SCB->ICSR = (1 << 28);	// Set pending PendSV service (switch)
}

void __machine_reset()
{
#ifdef DEBUG
	__BKPT(0);
#endif
	NVIC_SystemReset();
}

void __kernel_panic()
{
	__machine_reset();
}
