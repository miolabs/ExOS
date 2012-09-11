#include "timer.h"
#include "list.h"
#include "syscall.h"
#include "panic.h"
#include "machine/time_hal.h"

static EXOS_LIST _timers;

void __timer_init()
{
	list_initialize(&_timers);

	hal_time_initialize(EXOS_TICK_MICROS);
}



static int _add_timer(unsigned long *args)
{
	EXOS_TIMER *timer = (EXOS_TIMER *)args[0];

#ifdef DEBUG
	if (NULL != list_find_node(&_timers, (EXOS_NODE *)timer))
		kernel_panic(KERNEL_ERROR_TIMER_ALREADY_IN_USE);
#endif

	int signal = __signal_alloc();
	if (signal < 0) 
		kernel_panic(KERNEL_ERROR_TIMER_NOT_AVAILABLE);

	timer->Owner = __running_thread;
	timer->Signal = signal;
	timer->State = EXOS_TIMER_READY;

	list_add_tail(&_timers, (EXOS_NODE *)timer);
	return 0;
}

int exos_timer_create(EXOS_TIMER *timer, unsigned long time, unsigned long period)
{
#ifdef DEBUG
	if (timer == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);

	timer->Node = (EXOS_NODE) { .Type = EXOS_NODE_TIMER };
#endif
	timer->Time = time;
	timer->Period = period;
	__kernel_do(_add_timer, timer);
}



int exos_timer_wait(EXOS_TIMER *timer)
{
#ifdef DEBUG
	if (timer == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
	if (timer->Node.Type != EXOS_NODE_TIMER)
		kernel_panic(KERNEL_ERROR_TIMER_NOT_FOUND);
	if (timer->Owner != __running_thread)
		kernel_panic(KERNEL_ERROR_CROSS_THREAD_OPERATION_NOT_PERMITTED);
#endif

	exos_signal_wait(1 << timer->Signal);
}


static inline void _rem_timer_cleanup(EXOS_TIMER *timer, EXOS_TIMER_STATE state)
{
#ifdef DEBUG
	if (timer == NULL || timer->Owner == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	__signal_free(timer->Owner, timer->Signal);

	list_remove((EXOS_NODE *)timer);
	timer->State = state;
}

static int _rem_timer(unsigned long *args)
{
	EXOS_TIMER *timer = (EXOS_TIMER *)args[0];

	if (timer->State == EXOS_TIMER_READY)
	{
#ifdef DEBUG
		if (NULL == list_find_node(&_timers, (EXOS_NODE *)timer))
			kernel_panic(KERNEL_ERROR_TIMER_NOT_FOUND);
#endif

		_rem_timer_cleanup(timer, EXOS_TIMER_ABORTED);
	}
	return 0;
}

void exos_timer_abort(EXOS_TIMER *timer)
{
#ifdef DEBUG
	if (timer == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
	if (timer->Node.Type != EXOS_NODE_TIMER)
		kernel_panic(KERNEL_ERROR_TIMER_NOT_FOUND);
	if (timer->Owner != __running_thread)
		kernel_panic(KERNEL_ERROR_CROSS_THREAD_OPERATION_NOT_PERMITTED);
#endif

	__kernel_do(_rem_timer, timer);
}



static int _signal_timer(unsigned long *args)
{
	EXOS_TIMER *timer = (EXOS_TIMER *)args[0];
#ifdef DEBUG
	if (NULL == list_find_node(&_timers, (EXOS_NODE *)timer))
		kernel_panic(KERNEL_ERROR_TIMER_NOT_FOUND);
#endif

	__signal_set(timer->Owner, 1 << timer->Signal);

	if (timer->Period == 0)
	{
		_rem_timer_cleanup(timer, EXOS_TIMER_FINISHED);
	}
	else
	{
		timer->Time = timer->Period;
	}
	return  0;
}

void __kernel_tick()
{
	EXOS_NODE *node = LIST_HEAD(&_timers)->Succ;
	while(node != LIST_TAIL(&_timers))
	{
		EXOS_TIMER *timer = (EXOS_TIMER *)node;
		node = node->Succ;

		if (timer->Time != 0)
			timer->Time--;
		
		if (timer->Time == 0)
			__kernel_do(_signal_timer, timer);
	}
}

