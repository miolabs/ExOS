#ifndef EXOS_EVENT_H
#define EXOS_EVENT_H

#include <kernel/thread.h>
#include <kernel/signal.h>


typedef struct
{
	EXOS_LIST Handles;
	unsigned State;
} EXOS_EVENT;

void __set_event(EXOS_EVENT *event, int state);

void exos_event_create(EXOS_EVENT *event);
int exos_event_check(EXOS_EVENT *event, EXOS_WAIT_HANDLE *handle, unsigned long *pmask);
void exos_event_set(EXOS_EVENT *event);
void exos_event_reset(EXOS_EVENT *event);
int exos_event_wait(EXOS_EVENT *event, unsigned long timeout);
int exos_event_wait_multiple(EXOS_EVENT **events, int count, unsigned long timeout);
unsigned long exos_event_wait_signals(EXOS_EVENT *event, unsigned long mask, unsigned long timeout);

#endif // EXOS_EVENT_H
