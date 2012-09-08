#include "threads.h"
#include "list.h"
#include "panic.h"
#include "syscall.h"
<<<<<<< HEAD
#include "machine/hal.h"

// stack limits
extern unsigned long __stack_process_start__;
=======

// stack limits
extern unsigned long __stack_start__;
>>>>>>> 5c314a936eb4b82d55df2ee2c11c7f12fc824acb

// global running thread
EXOS_THREAD *__running_thread;

static EXOS_LIST _ready;
static EXOS_THREAD _system_thread;

static int _add_thread(unsigned long *args);

void __threads_init()
{
	list_initialize(&_ready);

	// initialize system thread
	_system_thread = (EXOS_THREAD) 
	{
<<<<<<< HEAD
		.StackStart = &__stack_process_start__,
=======
		.StackStart = (unsigned long)&__stack_start__,
>>>>>>> 5c314a936eb4b82d55df2ee2c11c7f12fc824acb
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

<<<<<<< HEAD
void threads_create(EXOS_THREAD *thread, int pri, void *stack, int stack_size, THREAD_FUNC entry, void *arg)
{
	stack_size = stack_size & ~7;	// align stack size

#ifdef DEBUG
	__mem_set(stack, stack + stack_size, 0xcc);
#endif
	
	void *stack_frame = __machine_init_thread_stack(stack + stack_size,
		(unsigned long)arg, (unsigned long)entry, (unsigned long)threads_join);

#ifdef DEBUG	
	if (stack_frame <= stack ||
		*(unsigned long *)stack != 0xcccccccc)
		kernel_panic(KERNEL_ERROR_STACK_INSUFFICIENT);
#endif

	*thread = (EXOS_THREAD)
	{
#ifdef DEBUG	
		.Node.Type = EXOS_NODE_THREAD,
#endif
		.Node.Priority = pri,
		.SP = stack_frame,
		.StackStart = stack,
	};

	__kernel_do(_add_thread, thread);
}

static int _join(unsigned long *args)
{
	EXOS_THREAD *thread = (EXOS_THREAD *)list_find_node(&_ready, (EXOS_NODE *)__running_thread);
	if (thread == NULL) kernel_panic(KERNEL_ERROR_THREAD_NOT_READY);

	thread->State = EXOS_THREAD_FINISHED;
	list_remove((EXOS_NODE *)thread);

	__machine_req_switch();
	return 0;
}

void threads_join()
{
	__kernel_do(_join);

	__kernel_panic();
}

static int _set_pri(unsigned long *args)
{
	int priority = (int)args[0];

	__running_thread->Node.Priority = priority;
	
	__machine_req_switch();
	return 0;
}

void threads_set_pri(int pri)
{
	__kernel_do(_set_pri, pri);
}
=======
>>>>>>> 5c314a936eb4b82d55df2ee2c11c7f12fc824acb
