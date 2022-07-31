#include "posix.h"
#include <stdlib.h>

#include <kernel/memory.h>

void *malloc(size_t size)
{
	return exos_mem_alloc(size, EXOS_MEMF_ANY);
}

void free(void *ptr)
{
	return exos_mem_free(ptr);
}

