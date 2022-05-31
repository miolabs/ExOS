#include <kernel/machine/hal.h>
#include <CMSIS/nrf51.h>
#include <support/board_hal.h>

void __machine_init()
{
#ifdef __ENABLE_HF_XTAL
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while(!NRF_CLOCK->EVENTS_HFCLKSTARTED);
#endif
#ifdef __ENABLE_LF_SYNTH_
	NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
	NRF_CLOCK->LFCLKSRC = 2; // Synth from HFCLK
	NRF_CLOCK->TASKS_LFCLKSTART = 1;
	while(!NRF_CLOCK->EVENTS_LFCLKSTARTED);
#endif

	hal_board_initialize();

	// set lowest priority for PendSV
	NVIC_SetPriority(PendSV_IRQn, 0xFF);	

	for (int i = 0; i <= 31; i++)
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

