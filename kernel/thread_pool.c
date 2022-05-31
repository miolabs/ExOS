#include "thread_pool.h"
#include <kernel/memory.h>
#include <kernel/panic.h>
#include <kernel/syscall.h>

int exos_thread_pool_create(EXOS_THREAD_POOL *pool)
{
	list_initialize(&pool->Threads);
	return 0;
}

static int _get_thread(unsigned long *args)
{
	EXOS_THREAD_POOL *pool = (EXOS_THREAD_POOL *)args[0];
	EXOS_THREAD **pthread = (EXOS_THREAD **)args[1];

	*pthread = (EXOS_THREAD *)list_rem_head(&pool->Threads);
	return 0;
}

EXOS_THREAD *exos_thread_pool_thread_create(EXOS_THREAD_POOL *pool, int pri, unsigned long stack_size, EXOS_THREAD_FUNC entry, void *arg)
{
#ifdef DEBUG
	if (pool == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	EXOS_THREAD *thread;
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

		thread = (EXOS_THREAD *)exos_mem_alloc(sizeof(EXOS_THREAD) + stack_size, EXOS_MEMF_CLEAR);
		if (thread == NULL) return NULL;

		stack = (void *)thread + sizeof(EXOS_THREAD);
	}
	else
	{
		stack = thread->StackStart;
#ifdef DEBUG
		if (stack != ((void *)thread + sizeof(EXOS_THREAD)))
			kernel_panic(KERNEL_ERROR_THREAD_POOL_CORRUPTED);
#endif
	}

	exos_thread_create(thread, pri, stack, stack_size, &pool->Threads, entry, arg);
	return thread;
}

int exos_thread_pool_cleanup(EXOS_THREAD_POOL *pool)
{
#ifdef DEBUG
	if (pool == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	while(1)
	{
		EXOS_THREAD *thread;
		__kernel_do(_get_thread, pool, &thread);
		if (thread == NULL) 
			break;
		
		exos_mem_free(thread);
	}
	return 0;
}

