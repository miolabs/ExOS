#ifndef EXOS_THREAD_POOL_H
#define EXOS_THREAD_POOL_H

#include <kernel/thread.h>

typedef struct
{
	list_t Threads;
} EXOS_THREAD_POOL;

int exos_thread_pool_create(EXOS_THREAD_POOL *pool);
EXOS_THREAD *exos_thread_pool_thread_create(EXOS_THREAD_POOL *pool, int pri, unsigned long stack_size, EXOS_THREAD_FUNC entry, void *arg);
int exos_thread_pool_cleanup(EXOS_THREAD_POOL *pool);

#endif // EXOS_THREAD_POOL_H


