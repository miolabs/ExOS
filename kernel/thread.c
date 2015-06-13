#include "thread.h"
#include "list.h"
#include "panic.h"
#include "syscall.h"
#include "signal.h"
#include "timer.h"
#include "memory.h"
#include "machine/hal.h"

// global running thread
EXOS_THREAD *__running_thread;

static EXOS_LIST _ready;
static EXOS_LIST _wait;
static EXOS_THREAD _system_thread;

static int _add_thread(unsigned long *args);

void __thread_init()
{
	list_initialize(&_ready);
	list_initialize(&_wait);

	// initialize system thread in process stack
	_system_thread = (EXOS_THREAD) 
	{
		.LocalStorage = (void *)__machine_tls_start - 8,
		.StackStart = (void *)__machine_process_start,
		.Node.Priority = -128,
#ifdef DEBUG
		.Node.Type = EXOS_NODE_THREAD,
#endif
	};

	*((unsigned long *)_system_thread.StackStart) = 0xcccccccc;	// mark stack limit

	__running_thread = &_system_thread;
	__kernel_do(_add_thread, &_system_thread);
}

static int _add_thread(unsigned long *args)
{
	EXOS_THREAD *thread = (EXOS_THREAD *)args[0];
	if (thread == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);

	thread->State = EXOS_THREAD_READY;
	list_enqueue(&_ready, (EXOS_NODE *)thread);
	
	__machine_req_switch();
	return 0;
}

EXOS_THREAD *__kernel_schedule()
{
#ifdef DEBUG
	EXOS_THREAD *current = __running_thread;
	if (current != NULL)
	{
		// check stack limit
		if (*((unsigned long *)current->StackStart) != 0xcccccccc) kernel_panic(KERNEL_ERROR_STACK_CORRUPTED);
		if (current->SP <= current->StackStart) kernel_panic(KERNEL_ERROR_STACK_OVERFLOW);
	}
#endif

	if (LIST_ISEMPTY(&_ready)) __kernel_panic();

	EXOS_THREAD *first = (EXOS_THREAD *)LIST_FIRST(&_ready);
#ifdef DEBUG
	if (first->Node.Type != EXOS_NODE_THREAD) kernel_panic(KERNEL_ERROR_WRONG_NODE);
#endif
	return first;
}

void exos_thread_create(EXOS_THREAD *thread, int pri, void *stack, unsigned stack_size, EXOS_LIST *recycler, EXOS_THREAD_FUNC entry, void *arg)
{
	stack_size = stack_size & ~7;	// align stack size

#ifdef DEBUG
	__mem_set(stack, stack + stack_size, 0xcc);
#endif
	
	void *stack_end = stack + stack_size;
	__machine_init_thread_local_storage(&stack_end);
	__machine_init_thread_stack(&stack_end,
		(unsigned long)arg, (unsigned long)entry, (unsigned long)exos_thread_exit);

#ifdef DEBUG	
	if (stack_end <= stack ||
		*(unsigned long *)stack != 0xcccccccc)
		kernel_panic(KERNEL_ERROR_STACK_INSUFFICIENT);
#endif

	*thread = (EXOS_THREAD)
	{
#ifdef DEBUG	
		.Node.Type = EXOS_NODE_THREAD,
#endif
		.Node.Priority = pri,
		.SP = stack_end,
		.StackStart = stack,
		.StackSize = stack_size,

		.SignalsReceived = 0,
		.SignalsWaiting = 0,
		.SignalsReserved = EXOS_SIGF_RESERVED_MASK,
		
		.RecycleList = recycler,
		.LocalStorage = stack_end - 8,	// NOTE: static tls skips first 8 bytes for DTV
		.ThreadContext = __running_thread != NULL ? 
			__running_thread->ThreadContext : NULL,
	};
	list_initialize(&thread->Joining);

	__kernel_do(_add_thread, thread);
}

static int _exit(unsigned long *args)
{
	EXOS_THREAD *thread = (EXOS_THREAD *)list_find_node(&_ready, (EXOS_NODE *)__running_thread);
	if (thread == NULL) kernel_panic(KERNEL_ERROR_THREAD_NOT_READY);

	__cond_signal_all(&thread->Joining, (void *)args[0]);

	thread->State = EXOS_THREAD_FINISHED;
	list_remove((EXOS_NODE *)thread);

	if (thread->RecycleList != NULL)
		list_add_tail(thread->RecycleList, (EXOS_NODE *)thread);

	__machine_req_switch();
	return 0;
}

void exos_thread_exit(void *result)
{
	__kernel_do(_exit, result);

	__kernel_panic();
}


static int _join(unsigned long *args)
{
	EXOS_THREAD *thread = (EXOS_THREAD *)args[0];
	EXOS_WAIT_HANDLE *handle = (EXOS_WAIT_HANDLE *)args[1];

	if (thread->State != EXOS_THREAD_FINISHED)
	{
		__cond_add_wait_handle(&thread->Joining, handle);
		__signal_wait(1 << handle->Signal);
		return -1;
	}
	return 0;
}

void *exos_thread_join(EXOS_THREAD *thread)
{
	EXOS_WAIT_HANDLE handle;
	while(__kernel_do(_join, thread, &handle) != 0);
	return handle.Result;
}

static int _set_pri(unsigned long *args)
{
	int priority = (int)args[0];

	EXOS_THREAD *thread = __running_thread;
	thread->Node.Priority = priority;
	if (thread->State == EXOS_THREAD_READY)
	{
		list_remove((EXOS_NODE *)thread);
		list_enqueue(&_ready, (EXOS_NODE *)thread);
	
		__machine_req_switch();
	}
	return 0;
}

void exos_thread_set_pri(int pri)
{
	__kernel_do(_set_pri, pri);
}

void exos_thread_sleep(unsigned ticks)
{
	EXOS_TIMER sleep_timer;
	exos_timer_create(&sleep_timer, ticks, 0, EXOS_SIGB_ABORT);
	exos_timer_wait(&sleep_timer);
}

void __thread_block()
{
	EXOS_THREAD *thread = __running_thread;
	if (thread->State == EXOS_THREAD_READY)
	{
#ifdef DEBUG
		if (NULL == list_find_node(&_ready, (EXOS_NODE *)thread))
			kernel_panic(KERNEL_ERROR_THREAD_NOT_READY);
#endif
		list_remove((EXOS_NODE *)thread);

		thread->State = EXOS_THREAD_WAIT;

		list_enqueue(&_wait, (EXOS_NODE *)thread);

		__machine_req_switch();
	}
}

void __thread_unblock(EXOS_THREAD *thread)
{
#ifdef DEBUG
	if (thread == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	if (thread->State == EXOS_THREAD_WAIT)
	{
#ifdef DEBUG
		if (NULL == list_find_node(&_wait, (EXOS_NODE *)thread))
			kernel_panic(KERNEL_ERROR_THREAD_NOT_WAITING);
#endif
		list_remove((EXOS_NODE *)thread);

		thread->State = EXOS_THREAD_READY;

		list_enqueue(&_ready, (EXOS_NODE *)thread);
	
		__machine_req_switch();
	}
}

void __thread_vacate()
{
	EXOS_THREAD *thread = __running_thread;
	if (thread->State == EXOS_THREAD_READY)
	{
#ifdef DEBUG
		if (NULL == list_find_node(&_ready, (EXOS_NODE *)thread))
			kernel_panic(KERNEL_ERROR_THREAD_NOT_READY);
#endif
		list_remove((EXOS_NODE *)thread);
		list_enqueue(&_ready, (EXOS_NODE *)thread);

		__machine_req_switch();
	}
}
