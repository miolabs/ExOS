#ifndef SUPPORT_MISC_POOLS_H
#define SUPPORT_MISC_POOLS_H

#include <kernel/list.h>
#include <kernel/mutex.h>
#include <stddef.h>

typedef struct
{
	mutex_t Mutex;
	list_t FreeItems;
} pool_t;

void pool_create(pool_t *pool, node_t *array, size_t item_size, unsigned item_count);
node_t *pool_allocate(pool_t *pool);
void pool_free(pool_t *pool, node_t *node);

#endif // SUPPORT_MISC_POOLS_H

