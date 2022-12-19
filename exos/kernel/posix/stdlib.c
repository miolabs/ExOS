#include "posix.h"
#include <stdlib.h>
#include <kernel/machine/hal.h>
#include <kernel/memory.h>
#include <kernel/panic.h>

int atoi(const char *str)
{
	return (int)strtol(str, (char **)NULL, 10);
}

long strtol(const char * restrict nptr, char ** restrict endptr, int base)
{
	unsigned off = 0;
	int value;
	switch(base)
	{
		case 10:
			off = __decl_int(nptr, &value);
			break; 
		default:
			kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
			break;
	}
	if (endptr != NULL)
		*endptr = (char *)nptr + off;
	return value;
}


void *malloc(size_t size)
{
	return exos_mem_alloc(size, EXOS_MEMF_ANY);
}

void free(void *ptr)
{
	return exos_mem_free(ptr);
}



