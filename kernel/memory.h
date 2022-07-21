#ifndef EXOS_MEMORY_H
#define EXOS_MEMORY_H

#include <kernel/list.h>
#include <kernel/mutex.h>

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
	node_t Node;
	EXOS_MEM_FLAGS Flags;
	void *StartAddress;
	unsigned long Size;
	mutex_t FreeListMutex;
	list_t FreeList;
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

int exos_mem_add_region(EXOS_MEM_REGION *region, void *start, void *end, int pri, EXOS_MEM_FLAGS flags);
void *exos_mem_alloc(unsigned long size, EXOS_MEM_FLAGS flags);
void exos_mem_free(void *addr);
void exos_mem_stats(EXOS_MEM_REGION *region, EXOS_MEM_STATS *stats);
EXOS_MEM_REGION *exos_mem_get_region(EXOS_MEM_FLAGS flags, int index);

void exos_mem_heap_stats(EXOS_MEM_STATS *stats);
unsigned long exos_mem_heap_avail();
void exos_mem_debug();

#endif // EXOS_MEMORY_H

