#include "memory.h"
#include "panic.h"
#include <kernel/machine/hal.h>
#include <kernel/mutex.h>
#include <support/board_hal.h>
#include <support/services/debug.h>

extern unsigned char __heap_start__, __heap_end__;

static EXOS_LIST _mem_list;
static EXOS_MUTEX _mem_lock;
static EXOS_MEM_REGION _heap_region;

void __mem_init()
{
	list_initialize(&_mem_list);
	exos_mutex_create(&_mem_lock);

#ifndef EXOS_HEAP_MEM_FLAGS
#define EXOS_HEAP_MEM_FLAGS EXOS_MEMF_ANY
#endif

	exos_mem_add_region(&_heap_region, &__heap_start__, &__heap_end__, -1, EXOS_HEAP_MEM_FLAGS);
    hal_board_add_memory();
}

static EXOS_MEM_HEADER *_init_block(EXOS_MEM_REGION *region, void *start, void *end)
{
	int size = end - start;
	EXOS_MEM_HEADER *header = (EXOS_MEM_HEADER *)start;
	*header = (EXOS_MEM_HEADER) {
		.Region = region,
		.Size = size - (sizeof(EXOS_MEM_HEADER) + sizeof(EXOS_MEM_FOOTER)) };
#ifdef DEBUG
	*(EXOS_NODE *)header->Contents = (EXOS_NODE) { .Type = EXOS_NODE_MEM_NODE };
#endif
	EXOS_MEM_FOOTER *footer = (EXOS_MEM_FOOTER *)(end - sizeof(EXOS_MEM_FOOTER));
	*footer = (EXOS_MEM_FOOTER) { .FreeSize = header->Size };
	return header;
}

int exos_mem_add_region(EXOS_MEM_REGION *region, void *start, void *end, int pri, EXOS_MEM_FLAGS flags)
{
	unsigned long size = end - start;

	if (size > (sizeof(EXOS_MEM_FOOTER) + sizeof(EXOS_MEM_HEADER) + sizeof(EXOS_MEM_HEADER)))
	{
		EXOS_MEM_FOOTER *head = (EXOS_MEM_FOOTER *)start;
		*head = (EXOS_MEM_FOOTER) { .FreeSize = 0 };

		EXOS_MEM_HEADER *tail = (EXOS_MEM_HEADER *)(end - sizeof(EXOS_MEM_HEADER));
		*tail = (EXOS_MEM_HEADER) { .Region = region, .Size = 0 };

		EXOS_MEM_HEADER *free = _init_block(region, start + sizeof(EXOS_MEM_FOOTER), tail);

		*region = (EXOS_MEM_REGION) {
			.Node = (EXOS_NODE) { 
#ifdef DEBUG
				.Type = EXOS_NODE_MEM_REGION,
#endif
				.Priority = pri },
			.Flags = flags,
			.StartAddress = start,
			.Size = size };
		list_initialize(&region->FreeList);
		list_add_tail(&region->FreeList, (EXOS_NODE *)free->Contents);
		exos_mutex_create(&region->FreeListMutex);

		exos_mutex_lock(&_mem_lock);
		list_enqueue(&_mem_list, (EXOS_NODE *)region);
		exos_mutex_unlock(&_mem_lock);
		return 1;
	}
	return 0;
}

static EXOS_MEM_HEADER *_find_room(EXOS_MEM_REGION *region, unsigned long size)
{
	FOREACH(node, &region->FreeList)
	{
#ifdef DEBUG
		if (node->Type != EXOS_NODE_MEM_NODE)
            kernel_panic(KERNEL_ERROR_MEMORY_CORRUPT);
#endif
		EXOS_MEM_HEADER *header = (EXOS_MEM_HEADER *)((void *)node - sizeof(EXOS_MEM_HEADER));
#ifdef DEBUG
		EXOS_MEM_FOOTER *footer = (EXOS_MEM_FOOTER *)((void *)node + header->Size);
		if (footer->FreeSize != header->Size)
			kernel_panic(KERNEL_ERROR_MEMORY_CORRUPT);
		if ((void *)header < region->StartAddress ||
			(void *)header >= (region->StartAddress + region->Size) ||
			((unsigned long)header & 0x3))
            kernel_panic(KERNEL_ERROR_MEMORY_CORRUPT);
#endif
		if (header->Size >= size)
			return header;
	}
	return NULL;
}

void *exos_mem_alloc(unsigned long size, EXOS_MEM_FLAGS flags)
{
	if (size == 0) return NULL;

    // align size
	size = (size + 15) & ~15;

	void *segment = NULL;

	exos_mutex_lock(&_mem_lock);
	FOREACH(node, &_mem_list)
	{
		EXOS_MEM_REGION *region = (EXOS_MEM_REGION *)node;
		if ((region->Flags & flags) == (flags & EXOS_MEMF_REGION))
		{
			exos_mutex_lock(&region->FreeListMutex);
			EXOS_MEM_HEADER *header = _find_room(region, size);
			if (header != NULL) 
			{
				if ((header->Size - size) >= (sizeof(EXOS_MEM_HEADER) + sizeof(EXOS_NODE) + sizeof(EXOS_MEM_FOOTER)))
				{
					void *end = (void *)header + (sizeof(EXOS_MEM_HEADER) + header->Size + sizeof(EXOS_MEM_FOOTER));
					void *split = (void*)header + (sizeof(EXOS_MEM_HEADER) + size + sizeof(EXOS_MEM_FOOTER));
					
					// mark used part as full
					header->Size = size;
					*(EXOS_MEM_FOOTER *)(split - sizeof(EXOS_MEM_FOOTER)) = (EXOS_MEM_FOOTER) { .FreeSize = 0 };
					
					// init unused part as new free block
					EXOS_MEM_HEADER *free = _init_block(region, split, end); 
                    list_insert((EXOS_NODE *)header->Contents, (EXOS_NODE *)free->Contents);
				}
				else
				{
					EXOS_MEM_FOOTER *footer = (EXOS_MEM_FOOTER *)(header->Contents + header->Size);
					*footer = (EXOS_MEM_FOOTER) { .FreeSize = 0 };
				}
				list_remove((EXOS_NODE *)header->Contents);

				if (flags & EXOS_MEMF_CLEAR) __mem_set(header->Contents, header->Contents + size, 0);
				segment = header->Contents;
			}
			exos_mutex_unlock(&region->FreeListMutex);

			if (segment != NULL) break;
		}
	}
	exos_mutex_unlock(&_mem_lock);
	return segment;
}

void exos_mem_free(void *addr)
{
	EXOS_MEM_HEADER *header = (EXOS_MEM_HEADER *)(addr - sizeof(EXOS_MEM_HEADER));
	EXOS_MEM_REGION *region = header->Region;
#ifdef DEBUG
	if (region->Node.Type != EXOS_NODE_MEM_REGION)
		kernel_panic(KERNEL_ERROR_MEMORY_CORRUPT);
#endif

	if (header->Size < sizeof(EXOS_NODE))
		kernel_panic(KERNEL_ERROR_MEMORY_CORRUPT);
	EXOS_MEM_FOOTER *footer = (EXOS_MEM_FOOTER *)(header->Contents + header->Size);
	if (footer->FreeSize != 0) 
		kernel_panic(KERNEL_ERROR_MEMORY_CORRUPT);

	EXOS_NODE *pred_node = NULL;

#ifdef MEMORY_PARANOID
	 __mem_set(header->Contents, header->Contents + header->Size, 0xdd);
#endif

	exos_mutex_lock(&region->FreeListMutex);

	// coalescence to linear successor
	EXOS_MEM_HEADER *next_header = (EXOS_MEM_HEADER *)((void *)footer + sizeof(EXOS_MEM_FOOTER));
	if (next_header->Size != 0)
	{
#ifdef DEBUG
		if (next_header->Region != region)
			kernel_panic(KERNEL_ERROR_MEMORY_CORRUPT);
#endif
		EXOS_MEM_FOOTER *next_footer = (EXOS_MEM_FOOTER *)(next_header->Contents + next_header->Size);
		if (next_footer->FreeSize == next_header->Size)
		{
#ifdef DEBUG
			if (((EXOS_NODE *)next_header->Contents)->Type != EXOS_NODE_MEM_NODE)
				kernel_panic(KERNEL_ERROR_MEMORY_CORRUPT);
#endif
			pred_node = ((EXOS_NODE *)next_header->Contents)->Pred;
			list_remove((EXOS_NODE *)next_header->Contents);
			
			footer = next_footer;
		}
	}

	// coalescence to linear predecessor
	EXOS_MEM_FOOTER *prev_footer = (EXOS_MEM_FOOTER *)((void *)header - sizeof(EXOS_MEM_FOOTER));
	if (prev_footer->FreeSize != 0)
	{
		EXOS_MEM_HEADER *prev_header = (EXOS_MEM_HEADER *)((void *)prev_footer - (prev_footer->FreeSize + sizeof(EXOS_MEM_HEADER)));
#ifdef DEBUG
		if (prev_header->Region != region  ||
			((EXOS_NODE *)prev_header->Contents)->Type != EXOS_NODE_MEM_NODE || 
			prev_header->Size != prev_footer->FreeSize)
			kernel_panic(KERNEL_ERROR_MEMORY_CORRUPT);
#endif
		pred_node = ((EXOS_NODE *)prev_header->Contents)->Pred;
		list_remove((EXOS_NODE *)prev_header->Contents);
		
		header = prev_header;
	}

	_init_block(region, header, (void *)footer + sizeof(EXOS_MEM_FOOTER));

	if (pred_node != NULL)
	{
		list_insert(pred_node, (EXOS_NODE *)header->Contents);
	}
	else
	{
		list_add_head(&region->FreeList, (EXOS_NODE *)header->Contents);
	}
	exos_mutex_unlock(&region->FreeListMutex);
}

void exos_mem_stats(EXOS_MEM_REGION *region, EXOS_MEM_STATS *stats)
{
#ifdef DEBUG
	if (region->Node.Type != EXOS_NODE_MEM_REGION)
		kernel_panic(KERNEL_ERROR_MEMORY_CORRUPT);
#endif

	unsigned long total = 0, count = 0, largest = 0;
	exos_mutex_lock(&region->FreeListMutex);
	FOREACH(node, &region->FreeList)
	{
		EXOS_MEM_HEADER *header = (EXOS_MEM_HEADER *)((void *)node - sizeof(EXOS_MEM_HEADER));
#ifdef DEBUG
		if (header->Region != region  ||
			node->Type != EXOS_NODE_MEM_NODE ||
			(void *)header < region->StartAddress) 
			kernel_panic(KERNEL_ERROR_MEMORY_CORRUPT);
		EXOS_MEM_FOOTER *footer = (EXOS_MEM_FOOTER *)(header->Contents + header->Size);
		if (header->Size != footer->FreeSize ||
			((void *)footer + sizeof(EXOS_MEM_FOOTER)) >= (region->StartAddress + region->Size))
			kernel_panic(KERNEL_ERROR_MEMORY_CORRUPT);

#ifdef MEMORY_DEBUG
		debug_printf("free fragment at 0x%lx, size=0x%lx (ends at 0x%lx)\r\n", 
			header, header->Size , header->Contents + header->Size + sizeof(EXOS_MEM_FOOTER));
#endif	
#endif	
		if (header->Size > largest) largest = header->Size; 
		total += header->Size;
		count++;
	}
	exos_mutex_unlock(&region->FreeListMutex);
	*stats = (EXOS_MEM_STATS) { .Free = total, .Fragments = count, .Largest = largest };
}

EXOS_MEM_REGION *exos_mem_get_region(EXOS_MEM_FLAGS flags, int index)
{
	EXOS_MEM_REGION *found = NULL;
	int i = 0;
	exos_mutex_lock(&_mem_lock);
	FOREACH(node, &_mem_list)
	{
		EXOS_MEM_REGION *region = (EXOS_MEM_REGION *)node;
		if ((region->Flags & flags) == (flags & EXOS_MEMF_REGION))
		{
			if (i == index)
			{
				found = region;
				break;
			}
			i++;
		}
	}
	exos_mutex_unlock(&_mem_lock);
	return found;
}


void exos_mem_heap_stats(EXOS_MEM_STATS *stats)
{
	exos_mem_stats(&_heap_region, stats);
}

unsigned long exos_mem_heap_avail()
{
	EXOS_MEM_STATS stats;
	exos_mem_stats(&_heap_region, &stats);
#ifdef MEMORY_DEBUG
    debug_printf("free mem 0x%lx in %d fragments, largest 0x%lx\r\n", stats.Free, stats.Fragments, stats.Largest);
#endif
	return stats.Free;
}

void exos_mem_debug()
{
	EXOS_MEM_STATS stats;
	EXOS_MEM_REGION *region;
	int i = 0;
	while(1)
	{
		region = exos_mem_get_region(EXOS_MEMF_ANY, i++);
		if (region == NULL) break;

		debug_printf("region 0x%lx (size = 0x%lx):\r\n", region->StartAddress, region->Size);
		exos_mem_stats(region, &stats);
		debug_printf("free mem 0x%lx in %d fragments, largest 0x%lx\r\n", stats.Free, stats.Fragments, stats.Largest);
	}
}

