#ifndef DM36X_GPIO_H
#define DM36X_GPIO_H

#include "types.h"


// NOTE: DIRection bits meaning is 0 = output, 1 = input
typedef enum
{
	GPIO_DIR_OUTPUT = 0,
	GPIO_DIR_INPUT = 1
} GPIO_DIR;

// Interrupt source pin vs ARM Interrupt Number 
// GIO0 GIO0 44
// GIO1 GIO1 45
// GIO2 GIO2 46
// GIO3 GIO3 47
// GIO4 GIO4 48
// GIO5 GIO5 49
// GIO6 GIO6 50
// GIO7 GIO7 51
// GIO8 GIO8 52
// GIO9 GIO9 53
// GIO10 GIO10 54
// GIO11 GIO11 55
// GIO12 GIO12 56
// GIO13 GIO13 57
// GIO14 GIO14 58
// GIO15 GIO15 59
// GPIO interrupt events are enabled in banks of 16 by setting the appropriate bit(s) in the GPIO interrupt
// per-bank enable register (BINTEN). For example, to enable bank 0 interrupts (events from GIO[15-0]), set
// bit 0 in BINTEN.

typedef volatile struct 
{
	unsigned long DIR;
	unsigned long OUT_DATA;
	unsigned long SET_DATA;
	unsigned long CLR_DATA;
	unsigned long IN_DATA;
	unsigned long SET_RIS_TRIG;
	unsigned long CLR_RIS_TRIG;
	unsigned long SET_FAL_TRIG;
	unsigned long CLR_FAL_TRIG;
	unsigned long INTSTAT;
} GPIO_PORT;

typedef volatile struct
{
	unsigned long PID;
	unsigned long Reserved04;
	unsigned long BINTEN;
	unsigned long Reserved0C;
	
	GPIO_PORT Port01;		// 10h
	GPIO_PORT Port23;		// 38h
	GPIO_PORT Port45;		// 60h
	GPIO_PORT Port6;		// 88h
} GPIO_MODULE;

typedef enum
{
	GPIO_BITEN_EN0 = (1<<0),
	GPIO_BITEN_EN6 = (1<<6),
} GPIO_BITEN;

// prototypes
void gpio_initialize();
void gpio_setup(int gio, GPIO_DIR dir, int value);
void gpio_set(int gio, int value);

#endif // DM36X_GPIO_H
