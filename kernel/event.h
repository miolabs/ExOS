#ifndef EXOS_EVENT_H
#define EXOS_EVENT_H

#include <kernel/thread.h>
#include <kernel/signal.h>

typedef enum
{
	EXOS_WAIT_PENDING,
	EXOS_WAIT_ABORTED,
	EXOS_WAIT_DONE,
} EXOS_WAIT_STATE;

typedef struct
{
	EXOS_NODE Node;
	EXOS_THREAD *Owner;
	EXOS_SIGNAL Signal;
	EXOS_WAIT_STATE State;
} EXOS_WAIT_HANDLE;

typedef struct
{
	EXOS_NODE Node;
	EXOS_LIST Handles;
	volatile int State;
} EXOS_EVENT;

#define EXOS_TIMEOUT_NEVER 0

int exos_event_create(EXOS_EVENT *event);
int exos_event_wait(EXOS_EVENT *event, unsigned long timeout);
int exos_event_wait_handle(EXOS_WAIT_HANDLE *handle, unsigned long timeout);
void exos_event_set(EXOS_EVENT *event);
void exos_event_reset(EXOS_EVENT *event);

#endif // EXOS_EVENT_H
