#include "dispatch.h"
#include <kernel/panic.h>
#include <kernel/timer.h>

void exos_dispatcher_context_create(dispatcher_context_t *context)
{
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);

	exos_mutex_create(&context->Lock);
	list_initialize(&context->Dispatchers);
	exos_event_create(&context->WakeEvent, EXOS_EVENTF_NONE);
}

void exos_dispatcher_create(dispatcher_t *dispatcher, event_t *event, dispatcher_callback_t callback, void *state)
{
	ASSERT(dispatcher != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(callback != NULL, KERNEL_ERROR_NULL_POINTER);

	*dispatcher = (dispatcher_t) { .Event = event,
		.State = DISPATCHER_CREATED,
		.Callback = callback, .CallbackState = state };
	exos_thread_create_wait_handle(&dispatcher->EventHandle);
	exos_thread_create_wait_handle(&dispatcher->TimerHandle);
}


static void _remove(dispatcher_context_t *context, dispatcher_t *dispatcher)
{
	if (dispatcher->Context != NULL)
	{
		ASSERT(dispatcher->Context == context, KERNEL_ERROR_KERNEL_PANIC);

		if (dispatcher->Flags & DISPATCHERF_HAS_TIMEOUT)
		{
			exos_thread_dispose_wait_handle(&dispatcher->TimerHandle);
			exos_timer_dispose(&dispatcher->Timer);
		}
		if (dispatcher->Event != NULL)
			exos_thread_dispose_wait_handle(&dispatcher->EventHandle);

		list_remove(&dispatcher->Node);
		dispatcher->Context = NULL;
	}
}

void exos_dispatcher_add(dispatcher_context_t *context, dispatcher_t *dispatcher, unsigned timeout)
{
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(dispatcher != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(dispatcher->State != DISPATCHER_UNKNOWN, KERNEL_ERROR_KERNEL_PANIC);

	exos_mutex_lock(&context->Lock);
	_remove(context, dispatcher);

	dispatcher->Flags = DISPATCHERF_NONE;
	if (timeout != 0)
	{
		dispatcher->Flags |= DISPATCHERF_HAS_TIMEOUT;
		exos_timer_create(&dispatcher->Timer, timeout, 0);
	}

	dispatcher->State = DISPATCHER_WAITING;
	dispatcher->Context = context;
	list_add_tail(&context->Dispatchers, &dispatcher->Node);
	exos_event_reset(&context->WakeEvent);
	exos_mutex_unlock(&context->Lock);
}

void exos_dispatcher_remove(dispatcher_context_t *context, dispatcher_t *dispatcher)
{
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(dispatcher != NULL, KERNEL_ERROR_NULL_POINTER);

	exos_mutex_lock(&context->Lock);
	_remove(context, dispatcher);
	exos_mutex_unlock(&context->Lock);
}

bool exos_dispatch(dispatcher_context_t *context, unsigned timeout)
{
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);
	dispatcher_t *selected = NULL;
	exos_wait_mask_t mask = 0;

	exos_mutex_lock(&context->Lock);
	FOREACH(n, &context->Dispatchers)
	{
		dispatcher_t *dispatcher = (dispatcher_t *)n;

		exos_wait_mask_t disp_mask = 0;
		if (dispatcher->Event != nullptr)
		{
			exos_wait_mask_t ev_mask = exos_event_add_handle(dispatcher->Event, &dispatcher->EventHandle);
			if (ev_mask == 0) 
			{
				dispatcher->State = DISPATCHER_DONE;
				selected = dispatcher;
				break;
			}
			disp_mask |= ev_mask;
		}
		if (dispatcher->Flags & DISPATCHERF_HAS_TIMEOUT)
		{
			exos_wait_mask_t tmr_mask = exos_event_add_handle(&dispatcher->Timer.Event, &dispatcher->TimerHandle);
			if (tmr_mask == 0)
			{
				dispatcher->State = DISPATCHER_TIMEOUT;
				selected = dispatcher;
				break;
			}
			disp_mask |= tmr_mask;
		}
		if (disp_mask == 0)
		{
			dispatcher->State = DISPATCHER_IMMEDIATE;
			selected = dispatcher;
			break;
		}
		mask |= disp_mask;
	}

#ifdef DEBUG
	dispatcher_t *selected1 = selected;
	dispatcher_t copy;
#endif
	if (selected != nullptr)
	{
#ifdef DEBUG
		copy = *selected;
#endif
		_remove(context, selected);
	}
	exos_mutex_unlock(&context->Lock);

	if (selected == nullptr)
	{
		exos_wait_handle_t context_handle;
		exos_thread_create_wait_handle(&context_handle);
		mask |= exos_event_add_handle(&context->WakeEvent, &context_handle);

		if (timeout != 0)
		{
			exos_timer_t timer;
			exos_wait_handle_t timer_handle;
			exos_timer_create(&timer, timeout, 0);
			exos_thread_create_wait_handle(&timer_handle);
			mask |= exos_event_add_handle(&timer.Event, &timer_handle);

			exos_thread_wait(mask);

			exos_thread_dispose_wait_handle(&timer_handle);
			exos_timer_dispose(&timer);
		}
		else
		{
			exos_thread_wait(mask);
		}

		exos_thread_dispose_wait_handle(&context_handle);

		exos_mutex_lock(&context->Lock);
		FOREACH(n, &context->Dispatchers)
		{
			dispatcher_t *dispatcher = (dispatcher_t *)n;

			if (dispatcher->Event != nullptr &&
				dispatcher->EventHandle.State == WAIT_HANDLE_DONE)
			{
				dispatcher->State = DISPATCHER_DONE;
				selected = dispatcher;
				break;
			}
			if ((dispatcher->Flags & DISPATCHERF_HAS_TIMEOUT) &&
				dispatcher->TimerHandle.State == WAIT_HANDLE_DONE)
			{
				dispatcher->State = DISPATCHER_TIMEOUT;
				selected = dispatcher;
				break;
			}
		}
		if (selected != nullptr)
			_remove(context, selected);

		exos_mutex_unlock(&context->Lock);
	}

	if (selected)
	{
		selected->Callback(context, selected);
		return true;
	}
	return false;
}

