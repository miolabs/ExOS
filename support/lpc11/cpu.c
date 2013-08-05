// LPC11xx sys/cpu support
// by Miguel Fides

#include "cpu.h"


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
/*
int cpu_cclk()
{
	return SystemCoreClock;
}

__attribute__((naked))
int cpu_trylock(unsigned char *lock, unsigned char value)
{
	__asm__ volatile (
		"ldrexb r2, [r0]\n\t"
		"cmp r2, r1\n\t"
		"itt ne\n\t"
		"strexbne r2, r1, [r0]\n\t"
		"cmpne r2, #0\n\t"
		"ite eq\n\t"
		"moveq r0, #1\n\t"
		"movne r0, #0\n\t"
		"bx lr\n\t");
}

void cpu_unlock(unsigned char *lock)
{
	*lock = 0;
}
*/
