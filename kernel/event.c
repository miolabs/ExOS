#include "event.h"
#include "signal.h"
#include "timer.h"
#include "syscall.h"
#include "panic.h"

int exos_event_create(EXOS_EVENT *event)
{
	list_initialize(&event->Handles);
	event->State = 0;
}


static int _add_wait_handle(unsigned long *args)
{
	EXOS_EVENT *event = (EXOS_EVENT *)args[0];
	EXOS_WAIT_HANDLE *handle = (EXOS_WAIT_HANDLE *)args[1];
	
	if (event->State) return -1;

	__cond_add_wait_handle(&event->Handles, handle);
	return 0;
}

static int _abort_wait(unsigned long *args)
{
	EXOS_EVENT *event = (EXOS_EVENT *)args[0];
	EXOS_WAIT_HANDLE *handle = (EXOS_WAIT_HANDLE *)args[1];
	
#ifdef DEBUG
	if (NULL == list_find_node(&event->Handles, (EXOS_NODE *)handle))
		__kernel_panic();
#endif

	__cond_rem_wait_handle(handle, EXOS_WAIT_ABORTED);
	return 0;
}

int exos_event_wait(EXOS_EVENT *event, unsigned long timeout)
{
#ifdef DEBUG
	if (event == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	EXOS_WAIT_HANDLE handle;
	if (__kernel_do(_add_wait_handle, event, &handle) == 0)
	{
		if (timeout != EXOS_TIMEOUT_NEVER)
		{
			EXOS_TIMER timer;
			exos_timer_create(&timer, timeout, 0, EXOS_SIGB_ABORT);
			unsigned long mask = exos_signal_wait((1 << handle.Signal) | EXOS_SIGF_ABORT);
			if (mask == EXOS_SIGF_ABORT)
			{
				__kernel_do(_abort_wait, event, &handle);
				return -1;	// timeout
			}
			exos_timer_abort(&timer);
		}
		else
		{
			exos_signal_wait(1 << handle.Signal);
		}
	}
	return 0;
}


static int _set_event(unsigned long *args)
{
	EXOS_EVENT *event = (EXOS_EVENT *)args[0];
	int state = (int)args[1];

	__cond_signal_all(&event->Handles);
	event->State = state;
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

