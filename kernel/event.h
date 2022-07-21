#ifndef EXOS_EVENT_H
#define EXOS_EVENT_H

#include <kernel/thread.h>
#include <kernel/signal.h>


typedef struct
{
	list_t Handles;
	unsigned State;
} event_t;

#ifdef EXOS_OLD
#define EXOS_EVENT event_t
#endif

void __set_event(event_t *event, int state);

void exos_event_create(event_t *event);
int exos_event_check(event_t *event, EXOS_WAIT_HANDLE *handle, unsigned long *pmask);
void exos_event_set(event_t *event);
void exos_event_reset(event_t *event);
int exos_event_wait(event_t *event, unsigned long timeout);
int exos_event_wait_multiple(event_t **events, int count, unsigned long timeout);
unsigned long exos_event_wait_signals(event_t *event, unsigned long mask, unsigned long timeout);

#endif // EXOS_EVENT_H
