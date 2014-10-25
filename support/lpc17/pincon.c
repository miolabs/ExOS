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

void pincon_setfunc(int port, int pin, int func)
{
	volatile unsigned long *pinsel = _pinsel[(port << 1) | (pin >> 4)];
	int shift = (pin & 0xF) << 1;
	unsigned long mask = 3 << shift;
	*pinsel = (*pinsel & ~mask) | (func << shift);
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
	iocon->Mode = mode;
	iocon->Hys = (flags & PINCONF_HYS) ? 1 : 0;
	iocon->Inv = (flags & PINCONF_INV) ? 1 : 0;
	iocon->ADMode = (flags & PINCONF_ANALOG) ? 1 : 0;
	iocon->OD = (flags & PINCONF_OPEN_DRAIN) ? 1 : 0;
}

#endif


// currently nothing