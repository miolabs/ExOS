#include <kernel/machine/hal.h>

__naked int __machine_trylock(unsigned char *lock, unsigned char value)
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

void __machine_unlock(unsigned char *lock)
{
	*lock = 0;
}

__always_inline inline void __machine_dsb(void)
{
  __asm__ volatile ("dsb 0xF":::"memory");
}