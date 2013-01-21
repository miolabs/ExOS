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

int cpu_trylock(unsigned char *lock, unsigned char value)
{
	int done;
	__asm__ volatile (
		"ldrexb %0, [%2]\n\t"
		"cmp %0, %1\n\t"
		"itt ne\n\t"
		"strexbne %0, %1, [%2]\n\t"
		"cmpne %0, #1\n\t"
		"ite eq\n\t"
		"moveq %0, #0\n\t"
		"movne %0, #1\n\t"
		: "=r" (done)
		: "r" (value), "r" (lock));
	return done;
}

void cpu_unlock(unsigned char *lock)
{
	*lock = 0;
}

