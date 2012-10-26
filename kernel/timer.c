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

void __timer_create_timer(EXOS_TIMER *timer, EXOS_SIGNAL signal)
{
#ifdef DEBUG
	if (NULL != list_find_node(&_timers, (EXOS_NODE *)timer))
		kernel_panic(KERNEL_ERROR_TIMER_ALREADY_IN_USE);

	timer->Node = (EXOS_NODE) { .Type = EXOS_NODE_TIMER };
#endif

	timer->Owner = __running_thread;
	timer->Signal = signal;
	timer->State = EXOS_TIMER_READY;

	list_add_tail(&_timers, (EXOS_NODE *)timer);
}

static int _add_timer(unsigned long *args)
{
	EXOS_TIMER *timer = (EXOS_TIMER *)args[0];
	EXOS_SIGNAL signal = (EXOS_SIGNAL)args[1];

	if (signal == EXOS_SIGB_NONE) 
	{
		signal = __signal_alloc();
	}
	else
	{
		__running_thread->SignalsReceived &= ~(1 << signal);
	}
		
	__timer_create_timer(timer, signal);
	return 0;
}

int exos_timer_create(EXOS_TIMER *timer, unsigned long time, unsigned long period, EXOS_SIGNAL signal)
{
#ifdef DEBUG
	if (timer == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	timer->Time = time;
	timer->Period = period;
	return __kernel_do(_add_timer, timer, signal);
}


void __timer_destroy_timer(EXOS_TIMER *timer)
{
#ifdef DEBUG
	if (timer == NULL || timer->Owner == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	if (!((1 << timer->Signal) & EXOS_SIGF_RESERVED_MASK))
		__signal_free(timer->Owner, timer->Signal);

	list_remove((EXOS_NODE *)timer);
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

		__timer_destroy_timer(timer);
		timer->State = EXOS_TIMER_ABORTED;
	}
	return 0;
}

void exos_timer_abort(EXOS_TIMER *timer)
{
#ifdef DEBUG
	if (timer == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
	if (timer->Node.Type != EXOS_NODE_TIMER)
		kernel_panic(KERNEL_ERROR_WRONG_NODE);
	if (timer->Owner != __running_thread)
		kernel_panic(KERNEL_ERROR_CROSS_THREAD_OPERATION);
#endif

	__kernel_do(_rem_timer, timer);
}


void exos_timer_wait(EXOS_TIMER *timer)
{
#ifdef DEBUG
	if (timer == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
	if (timer->Node.Type != EXOS_NODE_TIMER)
		kernel_panic(KERNEL_ERROR_TIMER_NOT_FOUND);
	if (timer->Owner != __running_thread)
		kernel_panic(KERNEL_ERROR_CROSS_THREAD_OPERATION);
#endif

	exos_signal_wait(1 << timer->Signal, EXOS_TIMEOUT_NEVER);
}


static unsigned long _time = 0;

static int _tick(unsigned long *args)
{
	EXOS_NODE *node = LIST_HEAD(&_timers)->Succ;
	while (node != LIST_TAIL(&_timers))
	{
		EXOS_TIMER *timer = (EXOS_TIMER *)node;
		node = node->Succ;

		if (timer->Time != 0)
			timer->Time--;
		
		if (timer->Time == 0)
		{
			__signal_set(timer->Owner, 1 << timer->Signal);
		
			if (timer->Period == 0)
			{
				__timer_destroy_timer(timer);
				timer->State = EXOS_TIMER_FINISHED;
			}
			else
			{
				timer->Time = timer->Period;
			}
		}
	}

	_time++;
	return  0;
}

void __kernel_tick()
{
	__kernel_do(_tick);
}

unsigned long exos_timer_time()
{
	return _time;
}

unsigned long exos_timer_elapsed(unsigned long time)
{
	return _time - time;
}
