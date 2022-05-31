#ifndef EXOS_MUTEX_H
#define EXOS_MUTEX_H

#include <kernel/signal.h>

typedef struct
{
	EXOS_LIST Handles;
	EXOS_THREAD *Owner;
	unsigned long Count;
} EXOS_MUTEX;

void exos_mutex_create(EXOS_MUTEX *mutex);
int exos_mutex_try(EXOS_MUTEX *mutex);
void exos_mutex_lock(EXOS_MUTEX *mutex);
void exos_mutex_unlock(EXOS_MUTEX *mutex);

#endif // EXOS_MUTEX_H
