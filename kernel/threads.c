#include "threads.h"
#include "list.h"
#include "panic.h"
#include "syscall.h"

// stack limits
extern unsigned long __stack_start__;

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
		.StackStart = (unsigned long)&__stack_start__,
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

