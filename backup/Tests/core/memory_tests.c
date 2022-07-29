#include "memory_tests.h"
#include <kernel/memory.h>
#include <kernel/panic.h>

static void _mem_check(EXOS_MEM_REGION *region)
{
#ifdef DEBUG
	if (region->Node.Type != EXOS_NODE_MEM_REGION)
		kernel_panic(KERNEL_ERROR_MEMORY_CORRUPT);
#endif

	EXOS_MEM_FOOTER *head = (EXOS_MEM_FOOTER *)region->StartAddress;
	EXOS_MEM_HEADER *tail = (EXOS_MEM_HEADER *)(region->StartAddress + region->Size - sizeof(EXOS_MEM_HEADER));
	if (head->FreeSize != 0 ||
		tail->Size != 0)
		kernel_panic(KERNEL_ERROR_MEMORY_CORRUPT);

	EXOS_MEM_HEADER *header = (EXOS_MEM_HEADER *)((void *)head + sizeof(EXOS_MEM_FOOTER));
	while(header != tail)
	{
		if (header->Region != region ||
        	header->Size < sizeof(EXOS_NODE))
			kernel_panic(KERNEL_ERROR_MEMORY_CORRUPT);
		
		EXOS_MEM_FOOTER *footer = (EXOS_MEM_FOOTER *)(header->Contents + header->Size);
		if (footer->FreeSize != header->Size &&
			footer->FreeSize != 0)
			kernel_panic(KERNEL_ERROR_MEMORY_CORRUPT);

		header = (EXOS_MEM_HEADER *)((void *)footer + sizeof(EXOS_MEM_FOOTER));
		if ((void *)header < region->StartAddress ||
			((void *)header + sizeof(EXOS_MEM_HEADER)) > (region->StartAddress + region->Size))
			kernel_panic(KERNEL_ERROR_MEMORY_CORRUPT);
	}

	EXOS_MEM_STATS stats;
	exos_mem_stats(region, &stats);
}

static int _alloc_free(int count)
{
	void *ptrs[count];

	int done = 0;
	while(done  < count)
	{
		void *ptr = exos_mem_alloc(1 << done, EXOS_MEMF_CLEAR);

		if (ptr != NULL)
			ptrs[done++] = ptr;
		else break;
	}

	while(done > 0)
	{
		void *ptr = ptrs[--done];

		exos_mem_free(ptr);
	}

	return 0;
}

static int _alloc_coalesce()
{
	void *ptr1 = exos_mem_alloc(16, EXOS_MEMF_CLEAR);
	void *ptr2 = exos_mem_alloc(16, EXOS_MEMF_CLEAR);
	void *ptr3 = exos_mem_alloc(16, EXOS_MEMF_CLEAR);

	exos_mem_free(ptr1);
	exos_mem_free(ptr2);
	exos_mem_free(ptr3);

	return 0;
}

int memory_tests()
{
	EXOS_MEM_REGION *region = exos_mem_get_region(EXOS_MEMF_ANY, 0);
	if (region != NULL)
	{
		EXOS_MEM_STATS stats;
		exos_mem_stats(region, &stats);

		int err = _alloc_free(8);
		_mem_check(region);
		if (err < 0)  return -1;

		err = _alloc_coalesce();
		_mem_check(region);
		if (err < 0) return -2;
	}
	return 0;
}



