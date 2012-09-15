#include "memory.h"
#include "panic.h"

extern unsigned char __heap_start__, __heap_end__;

static EXOS_LIST _mem_list;
static EXOS_MEM_REGION _heap_region;

void __mem_init()
{
	list_initialize(&_mem_list);

	exos_mem_add_region(&_heap_region, &__heap_start__, &__heap_end__, -1, 0);
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

void exos_mem_add_region(EXOS_MEM_REGION *region, void *start, void *end, int pri, unsigned long flags)
{
	unsigned long size = end -start;
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
	
	list_enqueue(&_mem_list, (EXOS_NODE *)region);
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
			((unsigned long)header & 0x7))
            kernel_panic(KERNEL_ERROR_MEMORY_CORRUPT);
#endif
		if (header->Size >= size)
			return header;
	}
	return NULL;
}

void *exos_mem_alloc(unsigned long size, unsigned long flags)
{
	if (size == 0) return NULL;

    // align size
	size = (size + 15) & ~15;

	FOREACH(node, &_mem_list)
	{
		EXOS_MEM_REGION *region = (EXOS_MEM_REGION *)node;
		if (region->Flags & flags)
		{
			EXOS_MEM_HEADER *header = _find_room(region, size);
			if (header != NULL) 
			{
				if (header->Size != size)
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
				list_remove((EXOS_NODE *)header->Contents);
				return header->Contents;
			}
		}
	}
	return NULL;
}

void exos_mem_free(void *addr)
{
	// TODO: check location for header/footer

	// TODO: add location to free list / coalesce to adjacent regions
}
