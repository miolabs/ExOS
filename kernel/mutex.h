#ifndef EXOS_MUTEX_H
#define EXOS_MUTEX_H

#include <kernel/thread.h>

typedef struct
{
	list_t Handles;
	exos_thread_t *Owner;
	unsigned long Count;
} mutex_t;

#ifdef EXOS_OLD
#define EXOS_MUTEX mutex_t
#endif

void exos_mutex_create(mutex_t *mutex);
int exos_mutex_try(mutex_t *mutex);
void exos_mutex_lock(mutex_t *mutex);
void exos_mutex_unlock(mutex_t *mutex);

#endif // EXOS_MUTEX_H
