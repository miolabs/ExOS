#include "signal.h"
#include "syscall.h"
#include "panic.h"
#include "timer.h"

void __signal_set(EXOS_THREAD *thread, unsigned long mask)
{
	thread->SignalsReceived |= mask;
	if (mask & thread->SignalsWaiting)
	{
		thread->SignalsWaiting &= ~mask;

		//TODO: use thread context to implement signal handlers
		__thread_unblock(thread);
	}
}

unsigned long __signal_wait(unsigned long mask)
{
	EXOS_THREAD *thread = __running_thread;
	unsigned long received = mask & thread->SignalsReceived;
	if (received)
	{
		thread->SignalsReceived &= ~received;
		return received;
	}
	thread->SignalsWaiting = mask;
	__thread_block();
	return 0;
}


static int _set_signal(unsigned long *args)
{
	EXOS_THREAD *thread = (EXOS_THREAD *)args[0];
	unsigned long mask = args[1];

	__signal_set(thread, mask);
	
	return 0;
}

void exos_signal_set(EXOS_THREAD *thread, unsigned long mask)
{
	__kernel_do(_set_signal, thread, mask);
}

static int _check_signal(unsigned long *args)
{
	unsigned long *pmask = (unsigned long *)args[0];
	unsigned long received = __signal_wait(*pmask);
	if (received)
	{
		*pmask = received;
		return 0;
	}
	return -1;
}

unsigned long exos_signal_wait(unsigned long mask, unsigned long timeout)
{
	if (timeout != EXOS_TIMEOUT_NEVER)
	{
		EXOS_TIMER timer;
		exos_timer_create(&timer, timeout, 0, EXOS_SIGB_ABORT);
		mask |= EXOS_SIGF_ABORT;

		while(__kernel_do(_check_signal, &mask) != 0);
		
		if (mask != EXOS_SIGF_ABORT)
		{
			exos_timer_abort(&timer);
		}
	}
	else
	{
		while(__kernel_do(_check_signal, &mask) != 0);
	}
	return mask;
}




EXOS_SIGNAL __signal_alloc()
{
	EXOS_THREAD *thread = __running_thread;
	unsigned long allocated = thread->SignalsReserved;

	for(int i = 0; i < 32; i++)
	{
		unsigned long mask = 1 << i;
		if ((allocated & mask) == 0) 
		{
			thread->SignalsReserved |= mask;
			thread->SignalsReceived &= ~mask;
			return i;
		}
	}
	kernel_panic(KERNEL_ERROR_NO_SIGNALS_AVAILABLE);
}

static int _sig_alloc(unsigned long *args)
{
	return __signal_alloc();
}

int exos_signal_alloc()
{
	return __kernel_do(_sig_alloc);
}



void __signal_free(EXOS_THREAD *thread, EXOS_SIGNAL signal)
{
#ifdef DEBUG
	if ((1 << signal) & EXOS_SIGF_RESERVED_MASK)
		__kernel_panic();
#endif

	thread->SignalsReserved &= ~(1 << signal);
}

static int _sig_free(unsigned long *args)
{
	EXOS_SIGNAL signal = (EXOS_SIGNAL)args[0];
	__signal_free(__running_thread, signal);
}

void exos_signal_free(EXOS_SIGNAL signal)
{
	__kernel_do(_sig_free, signal);
}



void __cond_add_wait_handle(list_t *list, EXOS_WAIT_HANDLE *handle)
{
#ifdef DEBUG
	handle->Node = (node_t) { .Type = EXOS_NODE_WAIT_HANDLE };
#endif

	handle->Owner = __running_thread;
	handle->Signal = __signal_alloc();

	list_add_tail(list, (node_t *)handle);
	handle->State = EXOS_WAIT_PENDING;
}

void __cond_rem_wait_handle(EXOS_WAIT_HANDLE *handle, EXOS_WAIT_STATE state)
{
#ifdef DEBUG
	if (handle == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
	if (handle->Node.Type != EXOS_NODE_WAIT_HANDLE)
		kernel_panic(KERNEL_ERROR_WRONG_NODE);
	if (state == EXOS_WAIT_PENDING)
		kernel_panic(KERNEL_ERROR_EVENT_WRONG_STATE);
#endif
	if (handle->State == EXOS_WAIT_PENDING)
	{
		handle->State = state;
		list_remove((node_t *)handle);
		__signal_free(handle->Owner, handle->Signal);
	}
}

int __cond_signal_all(list_t *list, void *result)
{
	int done = 0;
	node_t *node = LIST_HEAD(list)->Succ;
	while (node != LIST_TAIL(list))
	{
		EXOS_WAIT_HANDLE *handle = (EXOS_WAIT_HANDLE *)node;
		node = node->Succ;

		handle->Result = result;
		__signal_set(handle->Owner, 1 << handle->Signal);
		__cond_rem_wait_handle(handle, EXOS_WAIT_DONE);
		done++;
	}
	return done;
}


static int _abort_wait(unsigned long *args)
{
	EXOS_WAIT_HANDLE *handle = (EXOS_WAIT_HANDLE *)args[0];
	
	__cond_rem_wait_handle(handle, EXOS_WAIT_ABORTED);
	return 0;
}

void exos_cond_abort(EXOS_WAIT_HANDLE *handle)
{
	__kernel_do(_abort_wait, handle);
}



