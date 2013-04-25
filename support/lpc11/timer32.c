#include <support/pwm_hal.h>
#include "timer32.h"

static LPC_TMR_TypeDef *_modules[] = { LPC_TMR32B0, LPC_TMR32B1 };

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
		timer->MCR = TIMER_MCR_MR3R;
	}
	else
	{
		timer->MCR = 0;
	}
	timer->TCR = 1;
	return 1;
}

int hal_pwm_initialize(int module, int range, int rate)
{
	int done = timer32_initialize(module, range * rate, range);
	if (done)
	{
		LPC_TMR_TypeDef *timer = _modules[module];
		timer->MR0 = timer->MR1 = timer->MR2 = 0;
		timer->PWMC = 7;
	}
	return done; 
}

void hal_pwm_set_output(int module, int channel, int value)
{
	LPC_TMR_TypeDef *timer = _modules[module];
	switch(channel)
	{
		case 0:	timer->MR0 = value;	break;
		case 1:	timer->MR1 = value;	break;
		case 2:	timer->MR2 = value;	break;
	}

}

void hal_pwm_set_period(int module, int value)
{
}
