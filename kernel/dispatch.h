#ifndef EXOS_DISPATCH_H
#define EXOS_DISPATCH_H

#include <kernel/event.h>
#include <kernel/mutex.h>

typedef struct __EXOS_DISPATCHER_CONTEXT EXOS_DISPATCHER_CONTEXT;
typedef struct __EXOS_DISPATCHER EXOS_DISPATCHER;
typedef void (*EXOS_DISPATCHER_CALLBACK)(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher);

struct __EXOS_DISPATCHER
{
	EXOS_NODE Node;
	unsigned long Alarm;
	EXOS_EVENT *Event;
	EXOS_DISPATCHER_CALLBACK Callback;
	void *CallbackState;
};

struct __EXOS_DISPATCHER_CONTEXT
{
	EXOS_MUTEX Lock;
	EXOS_LIST Dispatchers;
	EXOS_EVENT WakeEvent;
	int Count;
};

void exos_dispatcher_context_create(EXOS_DISPATCHER_CONTEXT *context);
void exos_dispatcher_add(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher, unsigned long timeout); 
int exos_dispatcher_remove(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher);
void exos_dispatch(EXOS_DISPATCHER_CONTEXT *context, unsigned long timeout);

#endif // EXOS_DISPATCH_H

