// LPC23xx/24xx sys/cpu support
// by Miguel Fides

#include "cpu.h"

LPC_SC_TypeDef *LPC_SC = (LPC_SC_TypeDef *)0xE01FC000;
LPC_VIC_TypeDef *LPC_VIC = (LPC_VIC_TypeDef *)0xFFFFF000;

unsigned long SystemCoreClock = OSCILLATOR_CLOCK_FREQUENCY;

int cpu_cclk()
{
	return OSCILLATOR_CLOCK_FREQUENCY;
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

//int cpu_trylock(unsigned char *lock, unsigned char value)
//{
//	int done;
//	__asm__ volatile (
//		"ldrexb %0, [%2]\n\t"
//		"cmp %0, %1\n\t"
//		"itt ne\n\t"
//		"strexbne %0, %1, [%2]\n\t"
//		"cmpne %0, #1\n\t"
//		"ite eq\n\t"
//		"moveq %0, #0\n\t"
//		"movne %0, #1\n\t"
//		: "=r" (done)
//		: "r" (value), "r" (lock));
//	return done;
//}

void cpu_unlock(unsigned char *lock)
{
	*lock = 0;
}

void VIC_EnableIRQ(IRQn_Type irq)
{
	LPC_VIC->IntEnable = 1 << irq;
}

void VIC_DisableIRQ(IRQn_Type irq)
{
	LPC_VIC->IntEnClr = 1 << irq;
}


