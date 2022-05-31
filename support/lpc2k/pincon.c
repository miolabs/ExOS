// LPC17 GPIO Support
// by Miguel Fides

#include "pincon.h"

static volatile unsigned long *const _pinsel[] = {
	&LPC_PINCON->PINSEL0,
	&LPC_PINCON->PINSEL1,
	&LPC_PINCON->PINSEL2,
	&LPC_PINCON->PINSEL3,
	&LPC_PINCON->PINSEL4,
	&LPC_PINCON->PINSEL5,
	&LPC_PINCON->PINSEL6,
	&LPC_PINCON->PINSEL7,
	&LPC_PINCON->PINSEL8,
	&LPC_PINCON->PINSEL9,
	&LPC_PINCON->PINSEL10 };

static volatile unsigned long *const _pinmode[] = {
	&LPC_PINCON->PINMODE0,
	&LPC_PINCON->PINMODE1,
	&LPC_PINCON->PINMODE2,
	&LPC_PINCON->PINMODE3,
	&LPC_PINCON->PINMODE4,
	&LPC_PINCON->PINMODE5,
	&LPC_PINCON->PINMODE6,
	&LPC_PINCON->PINMODE7,
	&LPC_PINCON->PINMODE8,
	&LPC_PINCON->PINMODE9 };

void pincon_setfunc(int port, int pin, int func, int mode)
{
	volatile unsigned long *pinsel = _pinsel[(port << 1) | (pin >> 4)];
	int shift = (pin & 0xF) << 1;
	unsigned long mask = 3 << shift;
	*pinsel = (*pinsel & ~mask) | (func << shift);

	volatile unsigned long *pinmode = _pinmode[(port << 1) | (pin >> 4)];
	*pinmode = (*pinmode & ~mask) | (mode << shift);
}


