#include "hal.h"

#pragma GCC optimize(2)

__weak void __mem_copy(void *start, void *stop, const void *source)
 {
	if (source != start)
	{
		while(start != stop) *(unsigned char *)start++ = *(unsigned char *)source++;
	}
 }

__weak void __mem_set(void *start, void *stop, unsigned char stuff)
{
	while(start != stop) *(unsigned char *)start++ = stuff;
}


__weak unsigned int __str_copy(char *dst, char *src, unsigned int max_length)
{
	unsigned int done = 0;
	while(done < max_length)
	{
		char c = src[done];
		if (c == '\0') break;
		dst[done++] = c;
	}

	if (done < max_length) dst[done] = '\0';
	else if (done == max_length && done > 0) dst[done - 1] = '\0';
	return done;  
}

unsigned int __uint32_hexl(char *dst, unsigned long v)
{
	char buf[8];
	unsigned int length = 0;
	do
	{
		int digit = v & 0xF;
		v >>= 4;
		buf[length++] = (digit >= 10) ? digit + 'a' - 10 : digit + '0';
	} while(v != 0);

	for (int i = 0; i < length; i++) dst[i] = buf[length - i - 1];
	return length;
}

