#include "event.h"
#include "signal.h"
#include "timer.h"
#include "syscall.h"
#include "panic.h"

int exos_event_create(EXOS_EVENT *event)
{
#ifdef DEBUG
	event->Node = (EXOS_NODE) { .Type = EXOS_NODE_EVENT };
#endif

	list_initialize(&event->Handles);
	event->State = 0;
}

void __event_create_wait_handle(EXOS_WAIT_HANDLE *handle)
{
#ifdef DEBUG
	handle->Node = (EXOS_NODE) { .Type = EXOS_NODE_WAIT_HANDLE };
#endif

	handle->Owner = __running_thread;
	handle->Signal = __signal_alloc();
}

void __event_destroy_wait_handle(EXOS_WAIT_HANDLE *handle)
{
	__signal_free(handle->Owner, handle->Signal);
}

static int _add_wait_handle(unsigned long *args)
{
	EXOS_EVENT *event = (EXOS_EVENT *)args[0];
	EXOS_WAIT_HANDLE *handle = (EXOS_WAIT_HANDLE *)args[1];
	
	if (event->State) return -1;
	
	__event_create_wait_handle(handle);
	list_add_tail(&event->Handles, (EXOS_NODE *)handle);
	handle->State = EXOS_WAIT_PENDING;
	return 0;
}

static int _rem_wait_handle(unsigned long *args)
{
	EXOS_EVENT *event = (EXOS_EVENT *)args[0];
	EXOS_WAIT_HANDLE *handle = (EXOS_WAIT_HANDLE *)args[1];
	
#ifdef DEBUG
	if (NULL == list_find_node(&event->Handles, (EXOS_NODE *)handle))
		__kernel_panic();
#endif

	list_remove((EXOS_NODE *)handle);
	handle->State = EXOS_WAIT_ABORTED;
	__event_destroy_wait_handle(handle);
}

int exos_event_wait(EXOS_EVENT *event, unsigned long timeout)
{
#ifdef DEBUG
	if (event == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
	if (event->Node.Type != EXOS_NODE_EVENT)
		kernel_panic(KERNEL_ERROR_EVENT_NOT_FOUND);
#endif

	EXOS_WAIT_HANDLE handle;
	if (__kernel_do(_add_wait_handle, event, &handle) == 0)
	{
		if (timeout == EXOS_TIMEOUT_NEVER)
		{
			exos_signal_wait(1 << handle.Signal);
		}
		else
		{
			if (exos_event_wait_handle(&handle, timeout) < 0)
			{
				__kernel_do(_rem_wait_handle, event, &handle);
				return -1;	// timeout
			}
		}
	}
	return 0;
}

int exos_event_wait_handle(EXOS_WAIT_HANDLE *handle, unsigned long timeout)
{
	EXOS_TIMER timer;
	exos_timer_create(&timer, timeout, 0, EXOS_SIGB_ABORT);
	unsigned long mask = exos_signal_wait((1 << handle->Signal) | EXOS_SIGF_ABORT);
	if (mask == EXOS_SIGF_ABORT)
		return -1; // timeout

	exos_timer_abort(&timer);
	return 0;
}



static int _set_event(unsigned long *args)
{
	EXOS_EVENT *event = (EXOS_EVENT *)args[0];
	event->State = (int)args[1];

	EXOS_NODE *node = LIST_HEAD(&event->Handles)->Succ;
	while (node != LIST_TAIL(&event->Handles))
	{
		EXOS_WAIT_HANDLE *handle = (EXOS_WAIT_HANDLE *)node;
		node = node->Succ;

		list_remove((EXOS_NODE *)handle);

		__signal_set(handle->Owner, 1 << handle->Signal);
		handle->State = EXOS_WAIT_DONE;
		__event_destroy_wait_handle(handle);
	}
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

