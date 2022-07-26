#include "timer.h"
#include "list.h"
#include "syscall.h"
#include "panic.h"
#include <kernel/machine/time_hal.h>

static list_t _timers;
static volatile unsigned long _ticks = 0;

static void _tick_handler(unsigned elapsed);

void __timer_init()
{
	list_initialize(&_timers);

	hal_time_initialize(EXOS_TICK_MICROS);
}


static bool _enqueue(exos_timer_t *timer)
{
	exos_timer_t *pred = NULL;
	FOREACH(node, &_timers)
	{
		exos_timer_t *timer2 = (exos_timer_t *)node;
		if (timer->Time < timer2->Time)
			break;

		pred = timer2;
	}
	if (pred == NULL)
	{
		list_add_head(&_timers, &timer->Node);
		return true;
	}
	else
	{
		list_insert(&pred->Node, &timer->Node);
		return false;
	}
}

static int ___add_timer(unsigned long *args)
{
	unsigned elapsed = __machine_tick_elapsed();

	exos_timer_t *timer = (exos_timer_t *)args[0];
	timer->State = EXOS_TIMER_RUNNING;
	timer->Time += elapsed;
	if (_enqueue(timer))
		_tick_handler(elapsed);
	return 0;
}

void exos_timer_create(exos_timer_t *timer, unsigned time, unsigned period)
{
	ASSERT(timer != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(time != 0, KERNEL_ERROR_KERNEL_PANIC);
	timer->Node = (node_t) { .Type = EXOS_NODE_TIMER };
	timer->Time = time;
	timer->Period = period;
	exos_event_create(&timer->Event, EXOS_EVENTF_NONE);
	__kernel_do(___add_timer, timer);
}


static int ___rem_timer(unsigned long *args)
{
	exos_timer_t *timer = (exos_timer_t *)args[0];

	if (list_find_node(&_timers, &timer->Node))
	{
		list_remove(&timer->Node);

		unsigned elapsed = __machine_tick_elapsed();
		_tick_handler(elapsed);
	}
	return 0;
}

void exos_timer_dispose(exos_timer_t *timer)
{
	ASSERT(timer != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(timer->Node.Type == EXOS_NODE_TIMER, KERNEL_ERROR_WRONG_NODE);
	__kernel_do(___rem_timer, timer);
}

void exos_timer_wait(exos_timer_t *timer)
{
	ASSERT(timer != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(timer->Node.Type == EXOS_NODE_TIMER, KERNEL_ERROR_TIMER_NOT_FOUND);

	exos_event_wait(&timer->Event, EXOS_TIMEOUT_NEVER);
}

static void _tick_handler(unsigned elapsed)
{
	FOREACH(n, &_timers)
	{
		exos_timer_t *timer = (exos_timer_t *)n;
		ASSERT(timer != NULL, KERNEL_ERROR_KERNEL_PANIC);
		if (timer->Time <= elapsed)
		{
			list_remove(&timer->Node);
			if (timer->Period != 0)
			{
				// reload
				timer->Time = timer->Period;
				exos_event_reset(&timer->Event);
				_enqueue(timer);
			}
			else
			{
				timer->State = EXOS_TIMER_FINISHED;
				exos_event_set(&timer->Event);
			}
		}
		else
		{
			timer->Time -= elapsed;
		}
	}

	unsigned wait = 0;
	exos_timer_t *first = (exos_timer_t *)LIST_FIRST(&_timers);
	if (first != nullptr)
	{
		ASSERT(first->Time != 0, KERNEL_ERROR_KERNEL_PANIC);
		wait = first->Time;
	}
#ifndef EXOS_OLD_TICK_API
	__machine_tick_prepare(elapsed, wait, _tick_handler);
#endif
	_ticks += elapsed;
}


#ifdef EXOS_OLD_TICK_API

void __kernel_tick()
{
	_tick_handler(1);
}

unsigned __machine_tick_elapsed()
{
	return 0;
}

#endif

static inline unsigned _ticks_elapsed()
{
	unsigned elapsed = __machine_tick_elapsed();
	return elapsed + _ticks;
}

static int ___ticks_elapsed(unsigned long *args)
{
	unsigned long *ptotal = (unsigned long *)args[0];
	*ptotal = _ticks_elapsed();
	return 0;
}

static int ___uptime(unsigned long *args)
{
	static unsigned last_ticks;
	static unsigned seconds, fraction_ticks;

	unsigned long *puptime = (unsigned long *)args[0];
	
	unsigned add_ticks = _ticks_elapsed() - last_ticks;
	fraction_ticks += add_ticks;
	seconds += fraction_ticks / EXOS_TICK_FREQ;
	fraction_ticks = fraction_ticks % EXOS_TICK_FREQ;
	*puptime = seconds;
	return 0;
}

unsigned long exos_timer_time()
{
	unsigned long uptime;
	__kernel_do(___uptime, &uptime);
	return uptime;
}

unsigned long exos_timer_elapsed(unsigned long time)
{
	unsigned long uptime = exos_timer_time();
	return uptime - time;
}

