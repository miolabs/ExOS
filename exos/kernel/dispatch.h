#ifndef EXOS_DISPATCH_H
#define EXOS_DISPATCH_H

#include <kernel/event.h>
#include <kernel/timer.h>
#include <kernel/mutex.h>

typedef struct __dispatcher_context dispatcher_context_t;
typedef struct __dispatcher dispatcher_t;
typedef void (*dispatcher_callback_t)(dispatcher_context_t *context, dispatcher_t *dispatcher);
typedef enum
{
	DISPATCHER_UNKNOWN = 0,
	DISPATCHER_CREATED,
	DISPATCHER_WAITING,
	DISPATCHER_IMMEDIATE,
	DISPATCHER_DONE,
	DISPATCHER_TIMEOUT,
	DISPATCHER_REMOVED,
} dispatcher_state_t;

typedef enum
{
	DISPATCHERF_NONE = 0,
	DISPATCHERF_HAS_TIMEOUT = (1<<0),
} dispatcher_flags_t;

struct __dispatcher
{
	node_t Node;
	dispatcher_state_t State;
	dispatcher_flags_t Flags;
	dispatcher_context_t *Context;
	event_t *Event;
	exos_wait_handle_t EventHandle;
	exos_timer_t Timer;
	exos_wait_handle_t TimerHandle;
	dispatcher_callback_t Callback;
	void *CallbackState;
};

struct __dispatcher_context
{
	mutex_t Lock;
	list_t Dispatchers;
	event_t WakeEvent;
};

void exos_dispatcher_context_create(dispatcher_context_t *context);
void exos_dispatcher_create(dispatcher_t *dispatcher, event_t *event, dispatcher_callback_t callback, void *state);
void exos_dispatcher_add(dispatcher_context_t *context, dispatcher_t *dispatcher, unsigned timeout); 
void exos_dispatcher_remove(dispatcher_context_t *context, dispatcher_t *dispatcher);
bool exos_dispatch(dispatcher_context_t *context, unsigned timeout);


#ifdef EXOS_OLD
#define EXOS_DISPATCHER_CONTEXT dispatcher_context_t
#define EXOS_DISPATCHER dispatcher_t
#endif


#endif // EXOS_DISPATCH_H

