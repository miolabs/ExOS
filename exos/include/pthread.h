// ExOS posix layer

#ifndef __posix_pthread_h
#define __posix_pthread_h

#include <sys/types.h>
#include <time.h>

int pthread_create(pthread_t *thread, const pthread_attr_t *attrs, void *(* func)(void*), void *arg);
pthread_t pthread_self();
int pthread_equal(pthread_t t1, pthread_t t2);
int pthread_join(pthread_t thread, void **value_ptr);
void pthread_exit(void *value_ptr);


int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_timedwait(pthread_cond_t *cond, 
    pthread_mutex_t *mutex, const struct timespec *abstime);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);


int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);


#endif // __posix_types_h


