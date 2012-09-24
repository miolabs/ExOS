#include "mutex.h"
#include "syscall.h"
#include "panic.h"

void exos_mutex_create(EXOS_MUTEX *mutex)
{
	list_initialize(&mutex->Handles);
	mutex->Owner = NULL;
	mutex->Count = 0;
}

static int _try_lock(unsigned long *args)
{
	EXOS_MUTEX *mutex = (EXOS_MUTEX *)args[0];
	EXOS_WAIT_HANDLE *handle = (EXOS_WAIT_HANDLE *)args[1];

	if (mutex->Owner == __running_thread)
	{
		mutex->Count++;
	}
	else if (mutex->Owner == NULL)
	{
		mutex->Owner = __running_thread;
		mutex->Count = 0;
	}
	else
	{
		if (mutex->Handles.Tail != NULL)	// NOTE: complete static initialization
			list_initialize(&mutex->Handles);
		__cond_add_wait_handle(&mutex->Handles, handle);
		__signal_wait(1 << handle->Signal);
		return -1;
	}
	return 0;
}

void exos_mutex_lock(EXOS_MUTEX *mutex)
{
#ifdef DEBUG
	if (mutex == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	EXOS_WAIT_HANDLE handle;
	while(__kernel_do(_try_lock, mutex, &handle) != 0);
}


static int _unlock(unsigned long *args)
{
	EXOS_MUTEX *mutex = (EXOS_MUTEX *)args[0];

	if (mutex->Owner != __running_thread)
		kernel_panic(KERNEL_ERROR_CROSS_THREAD_OPERATION);

	if (mutex->Count == 0)
	{
		if (__cond_signal_all(&mutex->Handles) != 0)
			__thread_vacate(); // NOTE: force our thread to leave if there are others of the same priority to allow them to get the lock immediately 
		mutex->Owner = NULL;
	}
	else
	{
		mutex->Count--;
	}
}

void exos_mutex_unlock(EXOS_MUTEX *mutex)
{
#ifdef DEBUG
	if (mutex == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	__kernel_do(_unlock, mutex);
}

