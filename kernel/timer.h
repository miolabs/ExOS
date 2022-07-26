#ifndef EXOS_TIMER_H
#define EXOS_TIMER_H


#include <kernel/thread.h>
#include <kernel/event.h>

#ifndef EXOS_TICK_FREQ
#define EXOS_TICK_FREQ 1000	// 1ms / tick
#endif

#ifndef EXOS_TICK_MICROS
#define EXOS_TICK_MICROS (1000000 / EXOS_TICK_FREQ) 
#endif

typedef enum
{
	EXOS_TIMER_DETACHED = 0,
	EXOS_TIMER_RUNNING,
	EXOS_TIMER_FINISHED,
} exos_timer_state_t;

typedef struct
{
	node_t Node;
	exos_timer_state_t State;
	unsigned Period;
	unsigned Time;
	event_t Event;
} exos_timer_t;

void __timer_init();

void exos_timer_create(exos_timer_t *timer, unsigned time, unsigned period);
void exos_timer_wait(exos_timer_t *timer) __deprecated;
void exos_timer_dispose(exos_timer_t *timer);

unsigned long exos_timer_time();
unsigned long exos_timer_elapsed(unsigned long time);

#endif // EXOS_TIMER_H

