#include <pthread.h>
#include <errno.h>

#include <kernel/thread.h>
#include <kernel/memory.h>

static const pthread_attr_t _default_pthread_attrs = { 256, NULL };

int pthread_create(pthread_t *thread, const pthread_attr_t *attrs, void *(* func)(void*), void *arg)
{
	const pthread_attr_t *safe_attrs = attrs != NULL ? attrs : &_default_pthread_attrs;
	
	void *stack;
	pthread_info_t *info;
	if (safe_attrs->stack == NULL)
	{
		size_t alloc_size = safe_attrs->stack_size + sizeof(pthread_info_t); 
		void *alloc = exos_mem_alloc(alloc_size, EXOS_MEMF_ANY);
		if (alloc == NULL) return EAGAIN;
		info = (pthread_info_t *)(alloc + alloc_size - sizeof(pthread_info_t));
		stack = alloc;
	}
	else
	{
		info = (pthread_info_t *)exos_mem_alloc(sizeof(pthread_info_t), EXOS_MEMF_ANY);
		if (info == NULL) return EAGAIN;
		stack = safe_attrs->stack;
	}
	thread->info = info;

	exos_thread_create(&info->thread, safe_attrs->pri, 
		stack, safe_attrs->stack_size,
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


