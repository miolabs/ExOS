#include "swm.h"

volatile unsigned int *_assign = (unsigned int *)&(LPC_SWM->PINASSIGN0);
volatile unsigned int *_enable = (unsigned int *)&(LPC_SWM->PINENABLE0);

void lpc_swm_initialize()
{
	LPC_SYSCON->SYSAHBCLKCTRL0  |= CLKCTRL0_SWM;		// enable SWM clock
}

void lpc_swm_enable(LPC_SWME_PIN pin, int enable)
{
	unsigned int mask = 1 << (pin & 31);
	if (enable) _enable[pin >> 5] &= ~mask;
	else _enable[pin >> 5] |= mask;
}

void lpc_swm_assign(LPC_SWMA_PIN func, LPC_SWM_PIN pin)
{
	int shift = (func & 3) << 3;
	_assign[func >> 2] = (_assign[func >> 2] & ~(0xFF << shift)) | (pin << shift);
}



