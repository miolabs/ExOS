// LPC17xx sys/cpu support
// by Miguel Fides

#include "cpu.h"

#if (__TARGET_PROCESSOR > 1770)

int cpu_pclk(PCLK_PERIPH periph)
{
	// NOTE: periph is ignored
	int div = LPC_SC->PCLKSEL & 0x1F;
	return div == 0 ? 0 : SystemCoreClock / div; 
}

#else

int cpu_pclk(PCLK_PERIPH periph)
{
	int div = (periph < 16) ?
		((LPC_SC->PCLKSEL0 >> (periph << 1)) & 3) :
		((LPC_SC->PCLKSEL1 >> ((periph - 16) << 1)) & 3); 

	switch(div)
	{
		case 0:
			return SystemCoreClock >> 2;
		case 1:
			return SystemCoreClock;
		case 2:
			return SystemCoreClock >> 1;
		case 3:
			return SystemCoreClock >> 3;
	}
}

#endif



