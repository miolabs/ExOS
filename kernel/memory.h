#ifndef EXOS_MEMORY_H
#define EXOS_MEMORY_H

#include <kernel/list.h>

typedef struct
{
	EXOS_NODE Node;
	unsigned long Flags;
	void *StartAddress;
	unsigned long Size;
	EXOS_LIST FreeList;
} EXOS_MEM_REGION;

typedef struct __MEM_HEADER
{
	EXOS_MEM_REGION *Region;
	unsigned long Size;
	unsigned char Contents[0];
} EXOS_MEM_HEADER;

typedef struct 
{
	unsigned long FreeSize;
} EXOS_MEM_FOOTER;


void __mem_init();

void exos_mem_add_region(EXOS_MEM_REGION *region, void *start, void *end, int pri, unsigned long flags);

#endif // EXOS_MEMORY_H

