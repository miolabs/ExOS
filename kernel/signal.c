#include "signal.h"
#include "syscall.h"

void __signal_set(EXOS_THREAD *thread, unsigned long mask)
{
	thread->SignalsReceived |= mask;
	if (mask & thread->SignalsWaiting)
	{
		//TODO: use thread context to implement signal handlers
		__thread_unblock(thread);
	}
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
	EXOS_THREAD *thread = __running_thread;
	unsigned long *pmask = (unsigned long *)args[0];
	unsigned long mask = *pmask;

	unsigned long received = mask & thread->SignalsReceived;
	if (received)
	{
		thread->SignalsReceived &= ~received;
		*pmask = received;
		return 1;
	}
	thread->SignalsWaiting = mask;
	__thread_block();
	return 0;
}

unsigned long exos_signal_wait(unsigned long mask)
{
	while(!__kernel_do(_check_signal, &mask));
	return mask;
}



int __signal_alloc()
{
	EXOS_THREAD *thread = __running_thread;
	unsigned long allocated = thread->SignalsReserved;

	for(int i = EXOS_SIGB_RESERVED_COUNT; i < 32; i++)
	{
		unsigned long mask = 1 << i;
		if ((allocated & mask) == 0) 
		{
			thread->SignalsReserved |= mask;
			thread->SignalsReceived &= ~mask;
			return i;
		}
	}
	return -1;
}

static int _sig_alloc(unsigned long *args)
{
	return __signal_alloc();
}

int exos_signal_alloc()
{
	return __kernel_do(_sig_alloc);
}



void __signal_free(int signal)
{
	__running_thread->SignalsReserved &= ~(1 << signal);
}

static int _sig_free(unsigned long *args)
{
	int signal = (int)args[0];
	__signal_free(signal);
}

void exos_signal_free(int signal)
{
	__kernel_do(_sig_free, signal);
}


