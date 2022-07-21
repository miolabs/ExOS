#ifndef EXOS_DISPATCH_H
#define EXOS_DISPATCH_H

#include <kernel/event.h>
#include <kernel/mutex.h>

typedef struct __EXOS_DISPATCHER_CONTEXT dispatcher_context_t;
typedef struct __EXOS_DISPATCHER dispatcher_t;
typedef void (*EXOS_DISPATCHER_CALLBACK)(dispatcher_context_t *context, dispatcher_t *dispatcher);

struct __EXOS_DISPATCHER
{
	node_t Node;
	unsigned long Issued;
	unsigned long Timeout;
	event_t *Event;
	EXOS_DISPATCHER_CALLBACK Callback;
	void *CallbackState;
};

struct __EXOS_DISPATCHER_CONTEXT
{
	mutex_t Lock;
	list_t Dispatchers;
	event_t WakeEvent;
	int Count;
};

void exos_dispatcher_context_create(dispatcher_context_t *context);
void exos_dispatcher_create(dispatcher_t *dispatcher, event_t *event, EXOS_DISPATCHER_CALLBACK callback, void *state);
void exos_dispatcher_add(dispatcher_context_t *context, dispatcher_t *dispatcher, unsigned long timeout); 
int exos_dispatcher_remove(dispatcher_context_t *context, dispatcher_t *dispatcher);
void exos_dispatch(dispatcher_context_t *context, unsigned long timeout);


#ifdef EXOS_OLD
#define EXOS_DISPATCHER_CONTEXT dispatcher_context_t
#define EXOS_DISPATCHER dispatcher_t
#endif


#endif // EXOS_DISPATCH_H

