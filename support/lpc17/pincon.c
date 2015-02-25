// LPC17 Pin Connect Block Support
// by Miguel Fides

#include "pincon.h"

#if (__TARGET_PROCESSOR < 1770)

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

#else

static _IOCONbits *const _iocon[] = {
	(_IOCONbits *)&LPC_IOCON->P0_0,
	(_IOCONbits *)&LPC_IOCON->P1_0,
	(_IOCONbits *)&LPC_IOCON->P2_0,
	(_IOCONbits *)&LPC_IOCON->P3_0,
	(_IOCONbits *)&LPC_IOCON->P4_0,
	(_IOCONbits *)&LPC_IOCON->P5_0 };

void pincon_setfunc(int port, int pin, int func)
{
	_IOCONbits *iocon = _iocon[port] + pin;
	iocon->Func = func;
}

void pincon_setmode(int port, int pin, int mode, int flags)
{
	_IOCONbits *iocon = _iocon[port] + pin;
	*(unsigned long *)iocon = iocon->Func |
		(mode << 3) |
		((flags & PINCONF_HYS) ? (1<<5) : 0) |
		((flags & PINCONF_INV) ? (1<<6) : 0) |
		((flags & PINCONF_ANALOG) ? (1<<7) : 0) |
		((flags & PINCONF_OPEN_DRAIN) ? (1<<10) : 0);
}

#endif


// currently nothing