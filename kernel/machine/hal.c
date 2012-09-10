#include "hal.h"

#pragma GCC optimize(2)

__weak void __mem_copy(void *start, void *stop, void *source)
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

