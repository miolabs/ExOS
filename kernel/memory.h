#ifndef EXOS_MEMORY_H
#define EXOS_MEMORY_H

#include <kernel/list.h>

typedef enum
{
	EXOS_MEMF_ANY = 0,
	EXOS_MEMF_DMA = (1<<0),
	EXOS_MEMF_SHARED = (1<<1),
	EXOS_MEMF_REGION = (EXOS_MEMF_DMA | EXOS_MEMF_SHARED),
	EXOS_MEMF_CLEAR = (1<<4),
} EXOS_MEM_FLAGS;

typedef struct
{
	EXOS_NODE Node;
	EXOS_MEM_FLAGS Flags;
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

typedef struct
{
	unsigned long Free;
	unsigned long Fragments;
	unsigned long Largest;
} EXOS_MEM_STATS;

void __mem_init();

void exos_mem_add_region(EXOS_MEM_REGION *region, void *start, void *end, int pri, EXOS_MEM_FLAGS flags);
void *exos_mem_alloc(unsigned long size, EXOS_MEM_FLAGS flags);
void exos_mem_free(void *addr);
void exos_mem_stats(EXOS_MEM_REGION *region, EXOS_MEM_STATS *stats);
EXOS_MEM_REGION *exos_mem_get_region(EXOS_MEM_FLAGS flags, int index);

#endif // EXOS_MEMORY_H

