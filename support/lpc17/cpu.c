// LPC17xx sys/cpu support
// by Miguel Fides

#include "cpu.h"
#include <CMSIS/LPC17xx.h>

// for pre-CMSIS compatibility
int cpu_cclk()
{
	return SystemCoreClock;
}

int cpu_pclk(int cclk, int setting)
{
	switch(setting & 0x03)
	{
		case 0:
			return cclk >> 2;
		case 1:
		default:
			return cclk;
		case 2:
			return cclk >> 1;
		case 3:
			return cclk >> 3;
	}
}


