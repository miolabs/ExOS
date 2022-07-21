#ifndef EXOS_THREADS_H
#define EXOS_THREADS_H

#include <kernel/list.h>

typedef void *(* EXOS_THREAD_FUNC)(void *arg);

typedef enum
{
	EXOS_THREAD_READY = 0,
	EXOS_THREAD_WAIT,
	EXOS_THREAD_FINISHED,
} EXOS_THREAD_STATE;

typedef struct
{
	node_t Node;
	EXOS_THREAD_STATE State;
	void *SP;
	void *StackStart;
	unsigned long StackSize;
	void *TP;
	volatile unsigned long SignalsWaiting;
	volatile unsigned long SignalsReceived;
	volatile unsigned long SignalsReserved;
	list_t Joining;
	list_t *RecycleList;
	void *ThreadContext;	// FIXME: obsolete, will use TLS EABI
} EXOS_THREAD;

extern EXOS_THREAD *__running_thread;

void __thread_init();
EXOS_THREAD *__kernel_schedule();
void __thread_block();
void __thread_unblock(EXOS_THREAD *thread);
void __thread_vacate();

void exos_thread_create(EXOS_THREAD *thread, int pri, void *stack, unsigned stack_size, list_t *recycler, EXOS_THREAD_FUNC entry, void *arg);
void exos_thread_exit(void *result);
void *exos_thread_join(EXOS_THREAD *thread);

void exos_thread_set_pri(int pri);
void exos_thread_sleep(unsigned ticks);

#endif  // EXOS_THREADS_H

