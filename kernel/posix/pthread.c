#include <pthread.h>
#include <errno.h>
#include "posix.h"
#include <kernel/thread.h>
#include <kernel/memory.h>

static const pthread_attr_t _default_pthread_attrs = { 256, NULL };

int pthread_create(pthread_t *thread, const pthread_attr_t *attrs, void *(* func)(void*), void *arg)
{
	pthread_info_t *info = (pthread_info_t *)__running_thread;
	
	size_t stack_size = attrs != NULL && attrs->stack_size != 0 ? 
		attrs->stack_size : (size_t)info->thread.StackSize;

	void *stack;
	pthread_info_t *new_info;
	if (attrs == NULL || attrs->stack == NULL)
	{
		stack_size = (stack_size + 15) & ~15;
		void *alloc = exos_mem_alloc(stack_size + sizeof(pthread_info_t), EXOS_MEMF_ANY);
		if (alloc == NULL) return posix_set_error(EAGAIN);
		new_info = (pthread_info_t *)(alloc + stack_size);
		stack = alloc;
	}
	else
	{
		new_info = (pthread_info_t *)exos_mem_alloc(sizeof(pthread_info_t), EXOS_MEMF_ANY);
		if (info == NULL) return posix_set_error(EAGAIN);
		stack = attrs->stack;
	}
	thread->info = new_info;
	exos_thread_create(&new_info->thread, 
		attrs != NULL ? attrs->pri : info->thread.Node.Priority, 
		stack, stack_size,
		func, arg);
	return 0;
}

int pthread_join(pthread_t thread, void **value_ptr)
{
	pthread_info_t *info = (pthread_info_t *)thread.info;
	
	*value_ptr = exos_thread_join(&info->thread);
	return 0;
}

void pthread_exit(void *value_ptr)
{
	exos_thread_exit(value_ptr);
}

pthread_t pthread_self()
{
	pthread_info_t *info = (pthread_info_t *)__running_thread;
	return (pthread_t) { .info = info };
}

int pthread_equal(pthread_t t1, pthread_t t2)
{
	return t1.info == t2.info;
}


