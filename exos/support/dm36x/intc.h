#ifndef DM36X_INTC_H
#define DM36X_INTC_H

#include "types.h"

typedef volatile struct
{
	unsigned long long FIQ;
	unsigned long long IRQ;
	unsigned long FIQENTRY;
	unsigned long IRQENTRY;
	unsigned long long EINT;
	unsigned long INTCTL;
	unsigned long EABASE;
	unsigned long Reserved28;
	unsigned long Reserved2C;
	unsigned long INTPRI[8];
} INTC_CONTROLLER;

typedef enum
{
	INTC_PRI_FIQ_HIGH,
	INTC_PRI_FIQ_LOW,
	INTC_PRI_IRQ_HIGHEST,
	INTC_PRI_IRQ_HIGH,
	INTC_PRI_IRQ_MEDHIGH,
	INTC_PRI_IRQ_MEDLOW,
	INTC_PRI_IRQ_LOW,
	INTC_PRI_IRQ_LOWEST,
} INTC_PRI;


// prototypes
void intc_set_priority(int number, int enable, INTC_PRI pri);


#endif // DM36X_INTC_H
