#ifndef LPC17_PINCON_H
#define LPC17_PINCON_H

#include "cpu.h"

#if (__TARGET_PROCESSOR > 1770)
#define LPC177X_8X

typedef struct
{
	unsigned Func:3;
	unsigned Mode:2;
	unsigned Hys:1;
	unsigned Inv:1;
	unsigned ADMode:1;	// only A/D pins
	unsigned Filter:1;	// 
	unsigned Slew:1;
	unsigned OD:1;
	unsigned :4;
	unsigned DACEn:1;
} _IOCONbits;

#define IOCON_MODE_NOPULL	0
#define IOCON_MODE_PULL_DOWN	1
#define IOCON_MODE_PULL_UP	2
#define IOCON_MODE_REPEATER	3

#endif

#define PINCONF_HYS (1<<0)
#define PINCONF_INV (1<<1)
#define PINCONF_ANALOG (1<<2)
#define PINCONF_OPEN_DRAIN (1<<3)

// prototypes
void pincon_setfunc(int port, int pin, int func);
void pincon_setmode(int port, int pin, int mode, int flags);

#endif // LPC17_PINCON_H
