#include "thread.h"
#include "panic.h"
#include "syscall.h"
#include "timer.h"
#include "machine/hal.h"

// global running thread
exos_thread_t *__running_thread = nullptr;
exos_thread_t __main_thread;

#ifndef IDLE_THREAD_STACK
#define IDLE_THREAD_STACK 256	// NOTE: some platforms may need much more!
#endif

#ifndef MAIN_THREAD_PRI
#define MAIN_THREAD_PRI 0
#endif

static list_t _ready;
static list_t _wait;
static exos_thread_t _idle_thread;

static unsigned char _idle_thread_stack[IDLE_THREAD_STACK];

static int ___add_thread(unsigned long *args);
static void _resume(exos_wait_handle_t *handle);

void __thread_init()
{
	list_initialize(&_ready);
	list_initialize(&_wait);

	// create main thread from current context with MAIN_THREAD_PRI
	__main_thread.Node = (node_t) { .Priority = MAIN_THREAD_PRI, .Type = EXOS_NODE_THREAD };
	__main_thread.State = EXOS_THREAD_DETACHED;
	__main_thread.StackStart = __machine_stack_start;
	__main_thread.StackSize = 0;	// FIXME
	__main_thread.SP = nullptr;
	__main_thread.TP = __machine_tbss_start;
	list_initialize(&__main_thread.Pending);

	__running_thread = &__main_thread;
	__kernel_do(___add_thread, &__main_thread);

	// create idle thread with minimum priority
	exos_thread_create(&_idle_thread, -128, _idle_thread_stack, IDLE_THREAD_STACK, 
		(exos_thread_func_t)__machine_idle, nullptr);
}

static int ___add_thread(unsigned long *args)
{
	exos_thread_t *thread = (exos_thread_t *)args[0];
	ASSERT(thread->State == EXOS_THREAD_DETACHED, KERNEL_ERROR_KERNEL_PANIC);
	list_enqueue(&_ready, &thread->Node);
	thread->State = EXOS_THREAD_READY;
	__machine_req_switch();
}

exos_thread_t *__kernel_schedule()
{
	if (LIST_ISEMPTY(&_ready)) __kernel_panic();

	return (exos_thread_t *)LIST_FIRST(&_ready);
}	

static int ___rem_thread(unsigned long *args)
{
	exos_thread_t *thread = (exos_thread_t *)args[0];
	ASSERT(thread->State == EXOS_THREAD_READY, KERNEL_ERROR_KERNEL_PANIC);
    ASSERT(list_find_node(&_ready, &thread->Node), KERNEL_ERROR_LIST_CORRUPTED);
	list_remove(&thread->Node);
	thread->State = EXOS_THREAD_DETACHED;

	while(true)
	{
		exos_wait_handle_t *pending = nullptr;
		FOREACH(n, &thread->Pending)
		{
			exos_wait_handle_t *handle = (exos_wait_handle_t *)n;
			if (handle->Owner != thread)
			{
				pending = handle;
				_resume(pending);
				break;
			}
		}
		if (pending == nullptr)
			break;
	}

//	thread->State = EXOS_THREAD_READY;
//	list_enqueue(&_ready, (node_t *)thread);
	
	__machine_req_switch();
}

static void _exit()
{
	__kernel_do(___rem_thread, __running_thread);

	__kernel_panic();	// NOTE: should not get here
}


void exos_thread_create(exos_thread_t *thread, int pri, void *stack, unsigned stack_size, exos_thread_func_t entry, void *arg)
{
	stack_size = stack_size & ~7;	// align stack size

#ifdef DEBUG
	__mem_set(stack, stack + stack_size, 0xcc);
#endif
	
	void *stack_end = stack + stack_size;
	__machine_init_thread_local_storage(&stack_end);
	void *tp = stack_end - 8;	// NOTE: static tls skips first 8 bytes for DTV
	__machine_init_thread_stack(&stack_end,
		(unsigned)arg, (unsigned)entry, (unsigned)_exit);

#ifdef DEBUG	
	if (stack_end <= stack ||
		*(unsigned long *)stack != 0xcccccccc)
		kernel_panic(KERNEL_ERROR_STACK_INSUFFICIENT);
#endif

	*thread = (exos_thread_t) 
	{
		.Node = (node_t) { .Priority = pri, .Type = EXOS_NODE_THREAD },
		.State = EXOS_THREAD_DETACHED,
		.SP = stack_end,
		.StackStart = stack,
		.StackSize = stack_size,
		.TP = tp,
		.MaskUsed = 0,
		.MaskWait = 0,
#ifdef THREAD_DEBUG
		.Debugger = nullptr,
#endif
	};
	list_initialize(&thread->Pending);

	__kernel_do(___add_thread, thread);
}

static list_t *___join_cond(void *state)
{
	exos_thread_t *thread = (exos_thread_t *)state;
	ASSERT(thread != NULL, KERNEL_ERROR_NULL_POINTER);
	return (thread->State == EXOS_THREAD_READY || thread->State == EXOS_THREAD_WAIT) ? 
		&thread->Pending : NULL;
}

void *exos_thread_join(exos_thread_t *thread)
{
	ASSERT(thread != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(thread->Node.Type == EXOS_NODE_THREAD, KERNEL_ERROR_KERNEL_PANIC);

	exos_wait_handle_t handle;
	exos_thread_create_wait_handle(&handle);
	exos_wait_mask_t mask = exos_thread_add_wait_handle(&handle, ___join_cond, thread);
	if (mask != 0)
	{
		exos_thread_wait(mask);
		ASSERT(handle.State == WAIT_HANDLE_DONE, KERNEL_ERROR_KERNEL_PANIC);
		exos_thread_dispose_wait_handle(&handle);
	}
	ASSERT(thread->State == EXOS_THREAD_DETACHED, KERNEL_ERROR_KERNEL_PANIC);

	// FIXME: implement (void *) return value
	return NULL;
}

void exos_thread_exit(void *result)
{
	// TODO: re-implement recycler functionality
	_exit();
	
	// should never get here
	__kernel_panic();
}

void exos_thread_create_wait_handle(exos_wait_handle_t *handle)
{
	ASSERT(handle != nullptr, KERNEL_ERROR_NULL_POINTER);

	*handle = (exos_wait_handle_t) { .Node = { .Type = EXOS_NODE_WAIT_HANDLE },
		.State = WAIT_HANDLE_DETACHED };
}

static exos_wait_mask_t _alloc_mask()
{
	exos_wait_mask_t free = ~__running_thread->MaskUsed;
	exos_wait_mask_t mask = 1;
	do
	{
		if (mask & free)
		{
			__running_thread->MaskUsed |= mask;
			return mask;
		}
		mask <<= 1;
	} while(mask);

	__kernel_panic();
}

static int ___add_handle(unsigned long *args)
{
	exos_wait_handle_t *handle = (exos_wait_handle_t *)args[0];
	exos_wait_cond_t cond = (exos_wait_cond_t)args[1];
	void *state = (void *)args[2];

	list_t *list = cond(state);
	if (list != nullptr)
	{
		ASSERT(!list_find_node(list, &handle->Node), KERNEL_ERROR_LIST_ALREADY_CONTAINS_NODE);
		handle->Mask = _alloc_mask();
		handle->State = WAIT_HANDLE_WAITING;
		handle->Owner = __running_thread;
		list_add_tail(list, &handle->Node);
	}
}

exos_wait_mask_t exos_thread_add_wait_handle(exos_wait_handle_t *handle, exos_wait_cond_t cond, void *state)
{
	ASSERT(handle != nullptr, KERNEL_ERROR_NULL_POINTER);
	if (handle->State == WAIT_HANDLE_DETACHED)
	{
		__kernel_do(___add_handle, handle, cond, state);
		ASSERT(handle->Mask != 0 || handle->State ==  WAIT_HANDLE_DETACHED, KERNEL_ERROR_KERNEL_PANIC);
	}
	return handle->Mask;
}

static void _rem_handle(exos_wait_handle_t *handle)
{
	if (handle->State != WAIT_HANDLE_DETACHED)
	{
		list_remove(&handle->Node);
		handle->State = WAIT_HANDLE_DETACHED;
	}
}

static int ___dispose_handle(unsigned long *args)
{
	exos_wait_handle_t *handle = (exos_wait_handle_t *)args[0];
	_rem_handle(handle);

	if (handle->Owner != NULL)
	{
		ASSERT(0 != (handle->Owner->MaskUsed & handle->Mask), KERNEL_ERROR_KERNEL_PANIC);
		handle->Owner->MaskUsed ^= handle->Mask;
	}
	handle->Owner = NULL;
	handle->Mask = 0;
}

void exos_thread_dispose_wait_handle(exos_wait_handle_t *handle)
{
	ASSERT(handle != nullptr, KERNEL_ERROR_NULL_POINTER);
	if (handle->State != WAIT_HANDLE_DETACHED)
		__kernel_do(___dispose_handle, handle);
}

static void _resume(exos_wait_handle_t *handle)
{
	ASSERT(handle != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(handle->Node.Type == EXOS_NODE_WAIT_HANDLE, KERNEL_ERROR_KERNEL_PANIC);

	exos_thread_t *thread = handle->Owner;
	ASSERT(thread != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(thread->Node.Type == EXOS_NODE_THREAD, KERNEL_ERROR_KERNEL_PANIC);
	ASSERT(thread->State != EXOS_THREAD_DETACHED, KERNEL_ERROR_KERNEL_PANIC);

	_rem_handle(handle);
	list_enqueue(&thread->Pending, &handle->Node);
	handle->State = WAIT_HANDLE_DONE;

	if (thread->State == EXOS_THREAD_WAIT)
	{
		ASSERT(thread->MaskWait != 0, KERNEL_ERROR_KERNEL_PANIC);
		if (thread->MaskWait & handle->Mask)
		{
			list_remove(&thread->Node);
			list_enqueue(&_ready, &thread->Node);
			thread->State = EXOS_THREAD_READY;
#ifdef THREAD_DEBUG
			if (thread->Debugger != NULL)
				thread->Debugger(thread);
#endif
			__machine_req_switch();
		}
	}
}

static int ___resume(unsigned long *args)
{
	exos_wait_handle_t *handle = (exos_wait_handle_t *)args[0];
	_resume(handle);
}

void exos_thread_resume(exos_wait_handle_t *handle)
{
	ASSERT(handle != NULL, KERNEL_ERROR_NULL_POINTER);
	__kernel_do(___resume, handle);
}

static int ___resume_all(unsigned long *args)
{
	list_t *list = (list_t *)args[0];
	unsigned *pcount = (unsigned *)args[1];

	exos_wait_handle_t *handle;
	unsigned count = 0;
	while(handle = (exos_wait_handle_t *)LIST_FIRST(list), handle != NULL)
	{
		_resume(handle);
		count++;
	}
	*pcount = count;

	ASSERT(LIST_ISEMPTY(list), KERNEL_ERROR_KERNEL_PANIC);
}

unsigned exos_thread_resume_all(list_t *wait_handles)
{
	ASSERT(wait_handles != NULL, KERNEL_ERROR_NULL_POINTER);
	unsigned count = 0;
	__kernel_do(___resume_all, wait_handles, &count);
	return count;
}

static int ___wait(unsigned long *args)
{
	exos_thread_t *thread = __running_thread;
	ASSERT(thread->State == EXOS_THREAD_READY, KERNEL_ERROR_KERNEL_PANIC);
	exos_wait_mask_t mask = (exos_wait_mask_t)args[0];
	ASSERT(mask != 0, KERNEL_ERROR_KERNEL_PANIC);

	exos_wait_handle_t *pending = NULL;
	FOREACH(n, &thread->Pending)
	{
		exos_wait_handle_t *handle = (exos_wait_handle_t *)n;
		if (handle->Mask & mask)
		{
			pending = handle;
			break;
		}
	}
	if (pending == NULL)
	{
		list_remove(&thread->Node);
		list_enqueue(&_wait, &thread->Node);
		thread->State = EXOS_THREAD_WAIT;
		thread->MaskWait = mask;
#ifdef THREAD_DEBUG
		if (thread->Debugger != NULL)
			thread->Debugger(thread);
#endif
		__machine_req_switch();
	}
}

void exos_thread_wait(exos_wait_mask_t mask)
{
	__kernel_do(___wait, mask); 
}

void exos_thread_sleep(unsigned ticks)
{
	exos_timer_t sleep;
	exos_wait_handle_t handle;
	if (ticks != 0)
	{
		exos_thread_create_wait_handle(&handle);
		exos_timer_create(&sleep, ticks, 0);
		exos_wait_mask_t mask = exos_event_add_handle(&sleep.Event, &handle);
		if (mask != 0)
		{
			exos_thread_wait(mask);
			ASSERT(handle.State == WAIT_HANDLE_DONE, KERNEL_ERROR_KERNEL_PANIC);
			exos_thread_dispose_wait_handle(&handle);
		}
		ASSERT(sleep.State == EXOS_TIMER_FINISHED, KERNEL_ERROR_KERNEL_PANIC);
		exos_timer_dispose(&sleep);
	}
}

void exos_thread_set_debug_func(exos_thread_t *thread, void (*func)(exos_thread_t *thread))
{
#ifdef THREAD_DEBUG
	ASSERT(thread != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(thread->Debugger == nullptr, KERNEL_ERROR_KERNEL_PANIC);
	thread->Debugger = func;
#endif
}

