#include "thread_pool.h"
#include <kernel/memory.h>
#include <kernel/panic.h>
#include <kernel/syscall.h>

void exos_thread_pool_create(exos_thread_pool_t *pool)
{
	list_initialize(&pool->Threads);
}

static int _get_thread(unsigned long *args)
{
	exos_thread_pool_t *pool = (exos_thread_pool_t *)args[0];
	exos_thread_t **pthread = (exos_thread_t **)args[1];

	*pthread = (exos_thread_t *)list_rem_head(&pool->Threads);
	return 0;
}

exos_thread_t *exos_thread_pool_thread_create(exos_thread_pool_t *pool, int pri, unsigned long stack_size, exos_thread_func_t entry, void *arg)
{
#ifdef DEBUG
	if (pool == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	exos_thread_t *thread;
	__kernel_do(_get_thread, pool, &thread);

#ifdef DEBUG
	if (thread != NULL && thread->StackStart == NULL)
		kernel_panic(KERNEL_ERROR_THREAD_POOL_CORRUPTED);
#endif

	void *stack;
	if (thread == NULL || thread->StackSize != stack_size)
	{
		if (thread != NULL)
			exos_mem_free(thread);

		thread = (exos_thread_t *)exos_mem_alloc(sizeof(exos_thread_t) + stack_size, EXOS_MEMF_CLEAR);
		if (thread == NULL) return NULL;

		stack = (void *)thread + sizeof(exos_thread_t);
	}
	else
	{
		stack = thread->StackStart;
#ifdef DEBUG
		if (stack != ((void *)thread + sizeof(exos_thread_t)))
			kernel_panic(KERNEL_ERROR_THREAD_POOL_CORRUPTED);
#endif
	}

	exos_thread_create(thread, pri, stack, stack_size, /*&pool->Threads,*/ entry, arg);
	return thread;
}

void exos_thread_pool_cleanup(exos_thread_pool_t *pool)
{
#ifdef DEBUG
	if (pool == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	while(1)
	{
		exos_thread_t *thread;
		__kernel_do(_get_thread, pool, &thread);
		if (thread == NULL) 
			break;
		
		exos_mem_free(thread);
	}
}

