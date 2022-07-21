#include "pools.h"
#include <kernel/panic.h>

void pool_create(pool_t *pool, node_t *array, size_t item_size, unsigned item_count)
{
	exos_mutex_create(&pool->Mutex);
	list_initialize(&pool->FreeItems);

	if (array != nullptr && item_count != 0)
	{
		ASSERT(item_size >= sizeof(node_t), KERNEL_ERROR_KERNEL_PANIC);
		unsigned char *ptr = (unsigned char *)array;
		for(unsigned i = 0; i < item_count; i++)
		{
			list_add_tail(&pool->FreeItems, (node_t *)ptr);
			ptr += item_size;
		}
	}
}

node_t *pool_allocate(pool_t *pool)
{
	node_t *node;
	exos_mutex_lock(&pool->Mutex);

	node = LIST_FIRST(&pool->FreeItems);
	if (node != nullptr)
		list_remove(node);

	exos_mutex_unlock(&pool->Mutex);
	return node;
}

void pool_free(pool_t *pool, node_t *node)
{
	exos_mutex_lock(&pool->Mutex);
	ASSERT(!list_find_node(&pool->FreeItems, node), KERNEL_ERROR_KERNEL_PANIC);
	list_add_head(&pool->FreeItems, node);
	exos_mutex_unlock(&pool->Mutex);
}



