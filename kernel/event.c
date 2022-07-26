#include "event.h"
#include "timer.h"
#include "syscall.h"
#include "panic.h"

void exos_event_create(event_t *event, exos_event_flags_t flags)
{
	list_initialize(&event->Handles);
	event->State = false;
	event->Flags = flags;
}

static list_t *___event_cond(void *state)
{
	event_t *event = (event_t *)state;
	ASSERT(event != NULL, KERNEL_ERROR_NULL_POINTER);
	if (event->State)
	{
		if (event->Flags & EXOS_EVENTF_AUTORESET) 
			event->State = false;
		return nullptr;
	}
	else return &event->Handles;
}

exos_wait_mask_t exos_event_add_handle(event_t *event, exos_wait_handle_t *handle)
{
	ASSERT(event != NULL, KERNEL_ERROR_NULL_POINTER);
	return exos_thread_add_wait_handle(handle, ___event_cond, event);
}

static int ___event_set(unsigned long *args)
{
	event_t *event = (event_t *)args[0];
	unsigned count = exos_thread_resume_all(&event->Handles);
	event->State = (count == 0); 
}

void exos_event_set(event_t *event)
{
	ASSERT(event != NULL, KERNEL_ERROR_NULL_POINTER);
	
	if (event->Flags & EXOS_EVENTF_AUTORESET)
	{
		__kernel_do(___event_set, event);
	}
	else
	{
		event->State = true;
		exos_thread_resume_all(&event->Handles);
	}
}

void exos_event_reset(event_t *event)
{
	ASSERT(event != nullptr, KERNEL_ERROR_NULL_POINTER);
	event->State = false;
	exos_thread_resume_all(&event->Handles);
}

bool exos_event_wait(event_t *event, unsigned timeout)
{
	ASSERT(event != NULL, KERNEL_ERROR_NULL_POINTER);

	exos_wait_handle_t handle;
	exos_thread_create_wait_handle(&handle);
	bool done = true;
	exos_wait_mask_t event_mask = exos_event_add_handle(event, &handle);
	if (event_mask != 0)
	{
		if (timeout != 0)
		{
			exos_timer_t timer;
			exos_wait_handle_t timer_handle;
			exos_thread_create_wait_handle(&timer_handle);
			exos_timer_create(&timer, timeout, 0);
			exos_wait_mask_t timer_mask = exos_event_add_handle(&timer.Event, &timer_handle);
			if (timer_mask != 0)
			{
				exos_thread_wait(event_mask | timer_mask);
				ASSERT(handle.State == WAIT_HANDLE_DONE || timer_handle.State == WAIT_HANDLE_DONE, KERNEL_ERROR_KERNEL_PANIC);
				done = (handle.State == WAIT_HANDLE_DONE);
				exos_thread_dispose_wait_handle(&timer_handle);
			}
			exos_timer_dispose(&timer);
		}
		else
		{
			exos_thread_wait(event_mask);
			ASSERT(handle.State == WAIT_HANDLE_DONE, KERNEL_ERROR_KERNEL_PANIC);
		}
		exos_thread_dispose_wait_handle(&handle);
	}	
	return done;
}
