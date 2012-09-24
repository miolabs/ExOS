#include <pthread.h>
#include <errno.h>

#include <kernel/signal.h>
#include <kernel/mutex.h>
#include <kernel/panic.h>

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
	exos_mutex_create(&mutex->native_mutex);
	return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	if (mutex->native_mutex.Owner != NULL)
		kernel_panic(KERNEL_ERROR_MUTEX_IN_USE);
	return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
	exos_mutex_lock(&mutex->native_mutex);
	return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	return exos_mutex_try(&mutex->native_mutex) == 0 ? 0 : EBUSY;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	exos_mutex_unlock(&mutex->native_mutex);
	return 0;
}