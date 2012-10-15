#include "event.h"
#include "signal.h"
#include "timer.h"
#include "syscall.h"
#include "panic.h"

void exos_event_create(EXOS_EVENT *event)
{
	list_initialize(&event->Handles);
	event->State = 0;
}

static int _check_event(unsigned long *args)
{
	EXOS_EVENT *event = (EXOS_EVENT *)args[0];
	EXOS_WAIT_HANDLE *handle = (EXOS_WAIT_HANDLE *)args[1];
	
	if (!event->State)
	{
		__cond_add_wait_handle(&event->Handles, handle);
		return (1 << handle->Signal);
	}
	return 0;
}

int exos_event_check(EXOS_EVENT *event, EXOS_WAIT_HANDLE *handle, unsigned long *pmask)
{
#ifdef DEBUG
	if (event == NULL || handle == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	unsigned long mask = __kernel_do(_check_event, event, handle);
	if (mask == 0)
	{
		handle->State = EXOS_WAIT_DONE;
		return 1;
	}
	*pmask |= mask;
	return 0;
}

int exos_event_wait(EXOS_EVENT *event, unsigned long timeout)
{
#ifdef DEBUG
	if (event == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	EXOS_WAIT_HANDLE handle;
	unsigned long mask = __kernel_do(_check_event, event, &handle);
	if (mask != 0)
	{
		mask = exos_signal_wait(mask, timeout);
		if (mask == EXOS_SIGF_ABORT)
		{
			exos_cond_abort(&handle);
			return -1;	// timeout
		}
	}
	return 0;
}


void __set_event(EXOS_EVENT *event, int state)
{
	__cond_signal_all(&event->Handles);
	event->State = state;
}

static int _set_event(unsigned long *args)
{
	__set_event((EXOS_EVENT *)args[0], (int)args[1]);
	return 0;
}

void exos_event_set(EXOS_EVENT *event)
{
	__kernel_do(_set_event, event, 1);
}

void exos_event_reset(EXOS_EVENT *event)
{
	__kernel_do(_set_event, event, 0);
}

int exos_event_wait_multiple(EXOS_EVENT **events, int count, unsigned long timeout)
{
	EXOS_WAIT_HANDLE handles[count];
	unsigned long mask = 0;
	for(int i = 0; i < count; i++)
		exos_event_check(events[i], &handles[i], &mask);
	
	if (mask != 0)
		mask = exos_signal_wait(mask, timeout);

	for(int i = 0; i < count; i++)
	{
		EXOS_WAIT_HANDLE *handle = &handles[i];
		if (handle->State == EXOS_WAIT_PENDING)
			exos_cond_abort(handle);
	}
	return mask == EXOS_SIGF_ABORT ? -1 : 0;
}
