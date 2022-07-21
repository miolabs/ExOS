#include "event.h"
#include "signal.h"
#include "timer.h"
#include "syscall.h"
#include "panic.h"

void exos_event_create(event_t *event)
{
	list_initialize(&event->Handles);
	event->State = 0;
}

static int _check_event(unsigned long *args)
{
	event_t *event = (event_t *)args[0];
	EXOS_WAIT_HANDLE *handle = (EXOS_WAIT_HANDLE *)args[1];
	
	if (!event->State)
	{
		__cond_add_wait_handle(&event->Handles, handle);
		return (1 << handle->Signal);
	}
	return 0;
}

int exos_event_check(event_t *event, EXOS_WAIT_HANDLE *handle, unsigned long *pmask)
{
#ifdef DEBUG
	if (event == NULL || handle == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	unsigned long event_mask = __kernel_do(_check_event, event, handle);
	if (event_mask == 0)
	{
		handle->State = EXOS_WAIT_DONE;
		return 1;
	}
	*pmask |= event_mask;
	return 0;
}

int exos_event_wait(event_t *event, unsigned long timeout)
{
#ifdef DEBUG
	if (event == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	EXOS_WAIT_HANDLE handle;
	unsigned long event_mask = __kernel_do(_check_event, event, &handle);
	if (event_mask != 0)
	{
		event_mask = exos_signal_wait(event_mask, timeout);
		if (event_mask == EXOS_SIGF_ABORT)
		{
			exos_cond_abort(&handle);
			return -1;	// timeout
		}
	}
	return 0;
}


void __set_event(event_t *event, int state)
{
	__cond_signal_all(&event->Handles, NULL);
	event->State = state;
}

static int _set_event(unsigned long *args)
{
	__set_event((event_t *)args[0], (int)args[1]);
	return 0;
}

void exos_event_set(event_t *event)
{
#ifdef DEBUG
	if (event == NULL || event->Handles.Tail != NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	__kernel_do(_set_event, event, 1);
}

void exos_event_reset(event_t *event)
{
#ifdef DEBUG
	if (event == NULL || event->Handles.Tail != NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	__kernel_do(_set_event, event, 0);
}

int exos_event_wait_multiple(event_t **events, int count, unsigned long timeout)
{
	EXOS_WAIT_HANDLE handles[count];
	int done = 0;
	unsigned long mask = 0;
	for(int i = 0; i < count; i++)
	{
		if (exos_event_check(events[i], &handles[i], &mask))
			done++;
	}
	if (done == 0 && mask != 0)
		mask = exos_signal_wait(mask, timeout);

	for(int i = 0; i < count; i++)
	{
		EXOS_WAIT_HANDLE *handle = &handles[i];
		if (handle->State == EXOS_WAIT_PENDING)
			exos_cond_abort(handle);
	}
	return mask == EXOS_SIGF_ABORT ? -1 : 0;
}

unsigned long exos_event_wait_signals(event_t *event, unsigned long mask, unsigned long timeout)
{
#ifdef DEBUG
	if (event == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	EXOS_WAIT_HANDLE handle;
	unsigned long event_mask = __kernel_do(_check_event, event, &handle);
	if (event_mask != 0)
	{
		unsigned long rcv_mask  = exos_signal_wait(event_mask | mask, timeout);
		if (rcv_mask != event_mask)
		{
			exos_cond_abort(&handle); // cancel event handle
		}
		return rcv_mask;
	}
	return 0;
}
