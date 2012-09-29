#include "event.h"
#include "signal.h"
#include "timer.h"
#include "syscall.h"
#include "panic.h"

int exos_event_create(EXOS_EVENT *event, EXOS_EVENT_TYPE type)
{
	list_initialize(&event->Handles);
	event->State = 0;
	event->Type = type;
}


static int _check_event(unsigned long *args)
{
	EXOS_EVENT *event = (EXOS_EVENT *)args[0];
	EXOS_WAIT_HANDLE *handle = (EXOS_WAIT_HANDLE *)args[1];
	
	if (event->State) 
	{
		if (event->Type == EXOS_EVENT_AUTO_RESET)
			event->State = 0;
		return 1;
	}
	__cond_add_wait_handle(&event->Handles, handle);
	return 0;
}

int exos_event_wait(EXOS_EVENT *event, unsigned long timeout)
{
#ifdef DEBUG
	if (event == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	EXOS_WAIT_HANDLE handle;
	if (!__kernel_do(_check_event, event, &handle))
	{
		unsigned long mask = exos_signal_wait(1 << handle.Signal, timeout);
		if (mask == EXOS_SIGF_ABORT)
		{
			exos_cond_abort(&handle);
			return -1;	// timeout
		}
	}
	return 0;
}


static int _set_event(unsigned long *args)
{
	EXOS_EVENT *event = (EXOS_EVENT *)args[0];
	int state = (int)args[1];

	event->State = (__cond_signal_all(&event->Handles) != 0 
		&& event->Type == EXOS_EVENT_AUTO_RESET) ? 0 : state;
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

