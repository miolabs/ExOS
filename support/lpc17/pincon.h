#ifndef LPC17_PINCON_H
#define LPC17_PINCON_H

#include "cpu.h"

#if (__TARGET_PROCESSOR < 1770)

typedef struct
{
	unsigned P0_0:2; 
	unsigned P0_1:2; 
	unsigned P0_2:2; 
	unsigned P0_3:2; 
	unsigned P0_4:2; 
	unsigned P0_5:2; 
	unsigned P0_6:2; 
	unsigned P0_7:2; 
	unsigned P0_8:2; 
	unsigned P0_9:2; 
	unsigned P0_10:2; 
	unsigned P0_11:2; 
	unsigned P0_12:2; 
	unsigned P0_13:2; 
	unsigned P0_14:2; 
	unsigned P0_15:2; 
} _PINSEL0bits;

#define PINSEL0bits (*(_PINSEL0bits *) &LPC_PINCON->PINSEL0)
#define PINMODE0bits (*(_PINSEL0bits *) &LPC_PINCON->PINMODE0)

typedef struct
{
	unsigned P0_16:2; 
	unsigned P0_17:2; 
	unsigned P0_18:2; 
	unsigned P0_19:2; 
	unsigned P0_20:2; 
	unsigned P0_21:2; 
	unsigned P0_22:2; 
	unsigned P0_23:2; 
	unsigned P0_24:2; 
	unsigned P0_25:2; 
	unsigned P0_26:2; 
	unsigned P0_27:2; 
	unsigned P0_28:2; 
	unsigned P0_29:2; 
	unsigned P0_30:2; 
	unsigned P0_31:2; 
} _PINSEL1bits;

#define PINSEL1bits (*(_PINSEL1bits *) &LPC_PINCON->PINSEL1)
#define PINMODE1bits (*(_PINSEL1bits *) &LPC_PINCON->PINMODE1)

typedef struct
{
	unsigned P1_0:2; 
	unsigned P1_1:2; 
	unsigned P1_2:2; 
	unsigned P1_3:2; 
	unsigned P1_4:2; 
	unsigned P1_5:2; 
	unsigned P1_6:2; 
	unsigned P1_7:2; 
	unsigned P1_8:2; 
	unsigned P1_9:2; 
	unsigned P1_10:2; 
	unsigned P1_11:2; 
	unsigned P1_12:2; 
	unsigned P1_13:2; 
	unsigned P1_14:2; 
	unsigned P1_15:2; 
} _PINSEL2bits;

#define PINSEL2bits (*(_PINSEL2bits *) &LPC_PINCON->PINSEL2)

typedef struct
{
	unsigned P1_16:2; 
	unsigned P1_17:2; 
	unsigned P1_18:2; 
	unsigned P1_19:2; 
	unsigned P1_20:2; 
	unsigned P1_21:2; 
	unsigned P1_22:2; 
	unsigned P1_23:2; 
	unsigned P1_24:2; 
	unsigned P1_25:2; 
	unsigned P1_26:2; 
	unsigned P1_27:2; 
	unsigned P1_28:2; 
	unsigned P1_29:2; 
	unsigned P1_30:2; 
	unsigned P1_31:2; 
} _PINSEL3bits;

#define PINSEL3bits (*(_PINSEL3bits *) &LPC_PINCON->PINSEL3)

typedef struct
{
	unsigned P2_0:2; 
	unsigned P2_1:2; 
	unsigned P2_2:2; 
	unsigned P2_3:2; 
	unsigned P2_4:2; 
	unsigned P2_5:2; 
	unsigned P2_6:2; 
	unsigned P2_7:2; 
	unsigned P2_8:2; 
	unsigned P2_9:2; 
	unsigned P2_10:2; 
	unsigned P2_11:2; 
	unsigned P2_12:2; 
	unsigned P2_13:2; 
	unsigned P2_14:2; 
	unsigned P2_15:2; 
} _PINSEL4bits;

#define PINSEL4bits (*(_PINSEL4bits *) &LPC_PINCON->PINSEL4)

typedef struct
{
	unsigned P2_16:2; 
	unsigned P2_17:2; 
	unsigned P2_18:2; 
	unsigned P2_19:2; 
	unsigned P2_20:2; 
	unsigned P2_21:2; 
	unsigned P2_22:2; 
	unsigned P2_23:2; 
	unsigned P2_24:2; 
	unsigned P2_25:2; 
	unsigned P2_26:2; 
	unsigned P2_27:2; 
	unsigned P2_28:2; 
	unsigned P2_29:2; 
	unsigned P2_30:2; 
	unsigned P2_31:2; 
} _PINSEL5bits;

#define PINSEL5bits (*(_PINSEL5bits *) &LPC_PINCON->PINSEL5)

typedef struct
{
	unsigned P3_0:2; 
	unsigned P3_1:2; 
	unsigned P3_2:2; 
	unsigned P3_3:2; 
	unsigned P3_4:2; 
	unsigned P3_5:2; 
	unsigned P3_6:2; 
	unsigned P3_7:2; 
	unsigned P3_8:2; 
	unsigned P3_9:2; 
	unsigned P3_10:2; 
	unsigned P3_11:2; 
	unsigned P3_12:2; 
	unsigned P3_13:2; 
	unsigned P3_14:2; 
	unsigned P3_15:2; 
} _PINSEL6bits;

#define PINSEL6bits (*(_PINSEL6bits *) &LPC_PINCON->PINSEL6)

typedef struct
{
	unsigned P3_16:2; 
	unsigned P3_17:2; 
	unsigned P3_18:2; 
	unsigned P3_19:2; 
	unsigned P3_20:2; 
	unsigned P3_21:2; 
	unsigned P3_22:2; 
	unsigned P3_23:2; 
	unsigned P3_24:2; 
	unsigned P3_25:2; 
	unsigned P3_26:2; 
	unsigned P3_27:2; 
	unsigned P3_28:2; 
	unsigned P3_29:2; 
	unsigned P3_30:2; 
	unsigned P3_31:2; 
} _PINSEL7bits;

#define PINSEL7bits (*(_PINSEL7bits *) &LPC_PINCON->PINSEL7)

typedef struct
{
	unsigned P4_0:2; 
	unsigned P4_1:2; 
	unsigned P4_2:2; 
	unsigned P4_3:2; 
	unsigned P4_4:2; 
	unsigned P4_5:2; 
	unsigned P4_6:2; 
	unsigned P4_7:2; 
	unsigned P4_8:2; 
	unsigned P4_9:2; 
	unsigned P4_10:2; 
	unsigned P4_11:2; 
	unsigned P4_12:2; 
	unsigned P4_13:2; 
	unsigned P4_14:2; 
	unsigned P4_15:2; 
} _PINSEL8bits;

#define PINSEL8bits (*(_PINSEL8bits *) &LPC_PINCON->PINSEL8)

typedef struct
{
	unsigned P4_16:2; 
	unsigned P4_17:2; 
	unsigned P4_18:2; 
	unsigned P4_19:2; 
	unsigned P4_20:2; 
	unsigned P4_21:2; 
	unsigned P4_22:2; 
	unsigned P4_23:2; 
	unsigned P4_24:2; 
	unsigned P4_25:2; 
	unsigned P4_26:2; 
	unsigned P4_27:2; 
	unsigned P4_28:2; 
	unsigned P4_29:2; 
	unsigned P4_30:2; 
	unsigned P4_31:2; 
} _PINSEL9bits;

#define PINSEL9bits (*(_PINSEL9bits *) &LPC_PINCON->PINSEL9)


#else


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
