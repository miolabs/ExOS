#include "switch.h"
#include <CMSIS/LPC11xx.h>

#define SWITCH_PORT LPC_GPIO2
#define SWITCH_CHG_MASK (1<<8)
#define SWITCH_LOAD_MASK (1<<7)
#define SWITCH_STAY_MASK (1<<6)

int switch_initialize()
{
	unsigned long mask = SWITCH_CHG_MASK | SWITCH_LOAD_MASK | SWITCH_STAY_MASK;
	SWITCH_PORT->DIR |= mask;
	SWITCH_PORT->MASKED_ACCESS[mask] = 0;
	return 1;
}

void switch_charger(int state)
{
	SWITCH_PORT->MASKED_ACCESS[SWITCH_CHG_MASK] = state ? SWITCH_CHG_MASK : 0;
}

void switch_load(int state)
{
	SWITCH_PORT->MASKED_ACCESS[SWITCH_LOAD_MASK] = state ? SWITCH_LOAD_MASK : 0;
}
