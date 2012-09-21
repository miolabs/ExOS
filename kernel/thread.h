#ifndef EXOS_THREADS_H
#define EXOS_THREADS_H

#include <kernel/list.h>

typedef void (* EXOS_THREAD_FUNC)();

typedef enum
{
	EXOS_THREAD_READY = 0,
	EXOS_THREAD_WAIT,
	EXOS_THREAD_FINISHED,
} EXOS_THREAD_STATE;

typedef struct
{
	EXOS_NODE Node;
	EXOS_THREAD_STATE State;
	void *SP;
	void *StackStart;
	volatile unsigned long SignalsWaiting;
	volatile unsigned long SignalsReceived;
	volatile unsigned long SignalsReserved;
	void *ThreadContext;	// TODO
} EXOS_THREAD;

extern EXOS_THREAD *__running_thread;

void __thread_init();
EXOS_THREAD *__kernel_schedule();
void __thread_block();
void __thread_unblock(EXOS_THREAD *thread);
void __thread_vacate();

void exos_thread_create(EXOS_THREAD *thread, int pri, void *stack, unsigned stack_size, EXOS_THREAD_FUNC entry, void *arg);
void exos_thread_exit();
void exos_thread_set_pri(int pri);
void exos_thread_sleep(unsigned ticks);

#endif  // EXOS_THREADS_H

