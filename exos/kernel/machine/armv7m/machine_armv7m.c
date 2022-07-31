#include <kernel/machine/hal.h>
#include <string.h>

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


#pragma GCC optimize(1)

void __aeabi_memclr4(void *dest, size_t n)
{
	uint32_t *dest4 = (uint32_t *)dest;
	while(n != 0)
	{
		*dest4++ = 0;
		n -= 4;
	}
}

void __aeabi_memcpy4(void *dest, const void *src, size_t n)
{
	uint32_t *dest4 = (uint32_t *)dest;
	uint32_t *src4 = (uint32_t *)src;
	while(n != 0)
	{
		*dest4++ = *src4++;
		n -= 4;
	}
}

