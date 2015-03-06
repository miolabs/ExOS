// LPC23xx/24xx sys/cpu support
// by Miguel Fides

#include "cpu.h"

LPC_SC_TypeDef *LPC_SC = (LPC_SC_TypeDef *)0xE01FC000;
LPC_VIC_TypeDef *LPC_VIC = (LPC_VIC_TypeDef *)0xFFFFF000;

unsigned long SystemCoreClock = OSCILLATOR_CLOCK_FREQUENCY;

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

void VIC_EnableIRQ(IRQn_Type irq)
{
	LPC_VIC->IntEnable = 1 << irq;
}

void VIC_DisableIRQ(IRQn_Type irq)
{
	LPC_VIC->IntEnClr = 1 << irq;
}


