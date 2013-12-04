#include "timer16.h"

static LPC_TMR_TypeDef *_modules[] = { LPC_TMR16B0, LPC_TMR16B1 };

int timer32_initialize(int module, int freq, unsigned long period)
{
	switch(module)
	{
		case 0:
			LPC_SYSCON->SYSAHBCLKCTRL |= SYSAHBCLKCTRL_CT32B0;
			break;
		case 1:
			LPC_SYSCON->SYSAHBCLKCTRL |= SYSAHBCLKCTRL_CT32B1;
			break;
		default:
			return 0;
	}

	LPC_TMR_TypeDef *timer = _modules[module];

	timer->TCR = 0;
	timer->PR = (SystemCoreClock / freq) - 1;
	timer->CTCR = 0;
	timer->PWMC = 0;
	if (period != 0)
	{
		timer->MR3 = period;
		timer->MCR = TMR16_MCR_MR3R;
	}
	else
	{
		timer->MCR = 0;
	}
	timer->TCR = 1;
	return 1;
}