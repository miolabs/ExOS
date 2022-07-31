#ifndef EXOS_THREAD_POOL_H
#define EXOS_THREAD_POOL_H

#include <kernel/thread.h>

typedef struct
{
	list_t Threads;
} exos_thread_pool_t;

void exos_thread_pool_create(exos_thread_pool_t *pool);
exos_thread_t *exos_thread_pool_thread_create(exos_thread_pool_t *pool, int pri, unsigned long stack_size, exos_thread_func_t entry, void *arg);
void exos_thread_pool_cleanup(exos_thread_pool_t *pool);

#endif // EXOS_THREAD_POOL_H


