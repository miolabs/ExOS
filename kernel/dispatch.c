#include "dispatch.h"
#include <kernel/panic.h>
#include <kernel/timer.h>

void exos_dispatcher_context_create(EXOS_DISPATCHER_CONTEXT *context)
{
	exos_mutex_create(&context->Lock);
	list_initialize(&context->Dispatchers);
	context->Count = 0;
	exos_event_create(&context->WakeEvent);
}

void exos_dispatcher_create(EXOS_DISPATCHER *dispatcher, EXOS_EVENT *event, EXOS_DISPATCHER_CALLBACK callback, void *state)
{
	*dispatcher = (EXOS_DISPATCHER) { .Event = event, .Callback = callback, .CallbackState = state };
}

void exos_dispatcher_add(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher, unsigned long timeout)
{
#ifdef DEBUG
	if (context == NULL || dispatcher == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	exos_mutex_lock(&context->Lock);
#ifdef DEBUG
	if (list_find_node(&context->Dispatchers, (EXOS_NODE *)dispatcher))
		kernel_panic(KERNEL_ERROR_LIST_ALREADY_CONTAINS_NODE);
#endif
	dispatcher->Issued = exos_timer_time();
	dispatcher->Timeout = timeout;
	list_add_tail(&context->Dispatchers, (EXOS_NODE *)dispatcher);
	context->Count++;
	exos_mutex_unlock(&context->Lock);

	exos_event_reset(&context->WakeEvent);
}

static int _remove(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher)
{
	int done = 0;
	exos_mutex_lock(&context->Lock);
	if (list_find_node(&context->Dispatchers, (EXOS_NODE *)dispatcher))
	{
		list_remove((EXOS_NODE *)dispatcher);
        context->Count--;
		done = 1;
	}
	exos_mutex_unlock(&context->Lock);
	return done;
}

int exos_dispatcher_remove(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher)
{
	return _remove(context, dispatcher);
}

static void _call(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher)
{
	if (_remove(context, dispatcher) &&
		dispatcher->Callback != NULL)
		dispatcher->Callback(context, dispatcher);
	// NOTE: It is safe to reuse dispatcher struct or re-queue in the callback function
}

void exos_dispatch(EXOS_DISPATCHER_CONTEXT *context, unsigned long timeout)
{
	exos_mutex_lock(&context->Lock);
	
	EXOS_EVENT *array[context->Count + 1];
	int count = 0;
	array[count++] = &context->WakeEvent;

	EXOS_DISPATCHER *coming = NULL;
	int wait = timeout != EXOS_TIMEOUT_NEVER ? timeout : MAXINT;
	unsigned long time = exos_timer_time();
	FOREACH(node, &context->Dispatchers)
	{
		EXOS_DISPATCHER *dispatcher = (EXOS_DISPATCHER *)node;
		EXOS_EVENT *event = dispatcher->Event;
		if (event != NULL)
			array[count++] = event;

		int rem_time;
		if (dispatcher->Timeout != 0)
		{
			rem_time = dispatcher->Timeout + dispatcher->Issued - time;
		}
		else if (event == NULL) // EXOS_TIMEOUT_NEVER (0), without trigger event (NULL), means immediate dispatch!
		{
			rem_time = 0;
		}
		else continue;

		if (rem_time < wait)
		{
			wait = rem_time;
			coming = dispatcher;
		}
	}
	
	exos_mutex_unlock(&context->Lock);

	if (wait > 0)
	{
		if (exos_event_wait_multiple(array, count, wait) == 0)	// event occurred 
		{
			coming = NULL;
		}
	}
	if (coming != NULL)
	{
		_call(context, coming);
		return;
	}

	exos_mutex_lock(&context->Lock);
	FOREACH(node, &context->Dispatchers)
	{
		EXOS_DISPATCHER *dispatcher = (EXOS_DISPATCHER *)node;
		if (dispatcher->Event->State)
		{
			coming = dispatcher;
			break;
		}
	}
	exos_mutex_unlock(&context->Lock);

	if (coming != NULL)
		_call(context, coming);
}
