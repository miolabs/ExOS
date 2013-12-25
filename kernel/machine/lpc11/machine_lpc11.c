#include <kernel/machine/hal.h>
#include <CMSIS/LPC11xx.h>
#include <support/board_hal.h>

void __machine_init()
{
	hal_board_initialize();

	// set lowest priority for PendSV
	NVIC_SetPriority(PendSV_IRQn, 0xFF);	

	for (int i = 0; i <= 34; i++)
	{
		// set lowest priority for IRQ
		NVIC_SetPriority((IRQn_Type)i, 0xFF);
	}
	// disable system tick until timer initializes
	SysTick->CTRL = 0;
	SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk; // Clear pending SysTick
	__enable_irq();
}

void __machine_req_switch()
{
	SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;	// Set pending PendSV service (switch)
}

