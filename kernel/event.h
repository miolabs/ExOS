#ifndef EXOS_EVENT_H
#define EXOS_EVENT_H

#include <kernel/thread.h>
#include <kernel/signal.h>


typedef enum
{
	EXOS_EVENT_MANUAL_RESET = 0,
	EXOS_EVENT_AUTO_RESET,
} EXOS_EVENT_TYPE;

typedef struct
{
	EXOS_LIST Handles;
	unsigned State;
	EXOS_EVENT_TYPE Type;
} EXOS_EVENT;

int exos_event_create(EXOS_EVENT *event, EXOS_EVENT_TYPE flags);
int exos_event_wait(EXOS_EVENT *event, unsigned long timeout);
int exos_event_wait_handle(EXOS_WAIT_HANDLE *handle, unsigned long timeout);
void exos_event_set(EXOS_EVENT *event);
void exos_event_reset(EXOS_EVENT *event);

#endif // EXOS_EVENT_H
