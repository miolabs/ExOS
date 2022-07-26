#include <pthread.h>
#include <errno.h>
#include "posix.h"
#include <kernel/thread_pool.h>
#include <kernel/memory.h>

static const pthread_attr_t _default_pthread_attrs = { 256, NULL };

extern exos_thread_pool_t __posix_thread_pool;

int pthread_create(pthread_t *thread, const pthread_attr_t *attrs, void *(* func)(void*), void *arg)
{
	size_t stack_size = attrs != NULL && attrs->stack_size != 0 ? 
		attrs->stack_size : (size_t)__running_thread->StackSize;
	stack_size = (stack_size + 15) & ~15;

	int pri = attrs != NULL ? attrs->pri : __running_thread->Node.Priority;

	pthread_info_t *info = exos_thread_pool_thread_create(&__posix_thread_pool,
		pri, stack_size, func, arg);
	if (info == NULL)
		return posix_set_error(ENOMEM);

	*thread = (pthread_t) { .info = info };
	return 0;
}

int pthread_join(pthread_t thread, void **value_ptr)
{
	*value_ptr = exos_thread_join(thread.info);
	return 0;
}

void pthread_exit(void *value_ptr)
{
	exos_thread_exit(value_ptr);
}

pthread_t pthread_self()
{
	return (pthread_t) { .info = __running_thread };
}

int pthread_equal(pthread_t t1, pthread_t t2)
{
	return t1.info == t2.info;
}


