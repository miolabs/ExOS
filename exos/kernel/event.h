#ifndef EXOS_EVENT_H
#define EXOS_EVENT_H

#include <kernel/thread.h>
#include <stdbool.h>

typedef enum
{
	EXOS_EVENTF_NONE = 0,
	EXOS_EVENTF_AUTORESET = 1<<0,
} exos_event_flags_t;

typedef struct
{
	list_t Handles;
	bool State;
	exos_event_flags_t Flags;
} event_t;

#ifdef EXOS_OLD
#define EXOS_EVENT event_t
#endif

void exos_event_create(event_t *event, exos_event_flags_t flags);
exos_wait_mask_t exos_event_add_handle(event_t *event, exos_wait_handle_t *handle);

void exos_event_set(event_t *event);
void exos_event_reset(event_t *event);
bool exos_event_wait(event_t *event, unsigned timeout);

#endif // EXOS_EVENT_H
