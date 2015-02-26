#ifndef LPC17_PINCON_H
#define LPC17_PINCON_H

#include "cpu.h"

#if (__TARGET_PROCESSOR > 1770)

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

#define PINMODE_FLOAT		0
#define PINMODE_PULLDOWN	1
#define PINMODE_PULLUP		2
#define PINMODE_REPEATER	3

#define PINMODEF_HYS (1<<8)
#define PINMODEF_INV (1<<9)
#define PINMODEF_ANALOG (1<<10)
#define PINMODEF_OPEN_DRAIN (1<<11)

#else

#define PINMODE_PULLUP 0
#define PINMODE_REPEATER 1
#define PINMODE_FLOAT 2
#define PINMODE_PULLDOWN 3

#endif

// prototypes
void pincon_setfunc(int port, int pin, int func, int mode);

#endif // LPC17_PINCON_H
