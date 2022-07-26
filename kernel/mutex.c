#include "mutex.h"
#include "syscall.h"
#include "panic.h"

void exos_mutex_create(mutex_t *mutex)
{
	ASSERT(mutex != NULL, KERNEL_ERROR_NULL_POINTER);
	list_initialize(&mutex->Handles);
	mutex->Owner = NULL;
	mutex->Count = 0;
}

static list_t *___mutex_cond(void *state)
{
	mutex_t *mutex = (mutex_t *)state;
	ASSERT(mutex != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(__running_thread != NULL, KERNEL_ERROR_KERNEL_PANIC);

	if (mutex->Owner == NULL)
	{
		ASSERT(mutex->Count == 0, KERNEL_ERROR_KERNEL_PANIC);
		mutex->Count = 1;
		mutex->Owner = __running_thread;
		return NULL;
	}
	else if (mutex->Owner == __running_thread)
	{
		ASSERT(mutex->Count != 0, KERNEL_ERROR_KERNEL_PANIC);
		mutex->Count++;
		return NULL;
	}
	return &mutex->Handles;
}

void exos_mutex_lock(mutex_t *mutex)
{
	ASSERT(mutex != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(__running_thread != NULL, KERNEL_ERROR_KERNEL_PANIC);

	exos_wait_handle_t handle;
	exos_thread_create_wait_handle(&handle);
	do 
	{
		exos_wait_mask_t mask = exos_thread_add_wait_handle(&handle, ___mutex_cond, mutex);
		if (mask != 0)
		{
			exos_thread_wait(mask);
			ASSERT(handle.State == WAIT_HANDLE_DONE, KERNEL_ERROR_KERNEL_PANIC);
			exos_thread_dispose_wait_handle(&handle);
		}
	} while (mutex->Owner != __running_thread);
}

static int _unlock(unsigned long *args)
{
	mutex_t *mutex = (mutex_t *)args[0];

	if (mutex->Owner != __running_thread)
		kernel_panic(KERNEL_ERROR_CROSS_THREAD_OPERATION);
	ASSERT(mutex->Count != 0, KERNEL_ERROR_KERNEL_PANIC);
	mutex->Count--;

	if (mutex->Count == 0)
	{
		mutex->Owner = NULL;
		exos_thread_resume_all(&mutex->Handles);
	}
}

void exos_mutex_unlock(mutex_t *mutex)
{
	ASSERT(mutex != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(mutex->Owner == __running_thread, KERNEL_ERROR_CROSS_THREAD_OPERATION);
	__kernel_do(_unlock, mutex);
}

