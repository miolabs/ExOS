#ifndef EXOS_TIMER_H
#define EXOS_TIMER_H

#include <kernel/thread.h>
#include <kernel/signal.h>

#ifndef EXOS_TICK_FREQ
#define EXOS_TICK_FREQ 1000	// 1ms / tick
#endif

#ifndef EXOS_TICK_MICROS
#define EXOS_TICK_MICROS (1000000 / EXOS_TICK_FREQ) 
#endif

typedef enum
{
	EXOS_TIMER_UNKNOWN = 0,
	EXOS_TIMER_READY,
	EXOS_TIMER_ABORTED,
	EXOS_TIMER_FINISHED,
} EXOS_TIMER_STATE;

typedef struct
{
	EXOS_NODE Node;
	EXOS_THREAD *Owner;
	EXOS_SIGNAL Signal;
	EXOS_TIMER_STATE State;
	unsigned long Time;
	unsigned long Period;
} EXOS_TIMER;

void __timer_init();
void __timer_create_timer(EXOS_TIMER *timer, EXOS_SIGNAL signal);
void __timer_destroy_timer(EXOS_TIMER *timer);

int exos_timer_create(EXOS_TIMER *timer, unsigned long time, unsigned long period, EXOS_SIGNAL signal);
void exos_timer_wait(EXOS_TIMER *timer);
void exos_timer_abort(EXOS_TIMER *timer);

unsigned long exos_timer_time();
unsigned long exos_timer_elapsed(unsigned long time);

#endif // EXOS_TIMER_H

