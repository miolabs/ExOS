#ifndef EXOS_THREADS_H
#define EXOS_THREADS_H

#include <kernel/list.h>

typedef void (* THREAD_FUNC)();

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
} EXOS_THREAD;

void __threads_init();
EXOS_THREAD *__kernel_schedule();
void threads_create(EXOS_THREAD *thread, int pri, void *stack, int stack_size, THREAD_FUNC entry, void *arg);
void threads_join();
void threads_set_pri(int pri);

#endif  // EXOS_THREADS_H

