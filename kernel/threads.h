#ifndef EXOS_THREADS_H
#define EXOS_THREADS_H

#include <kernel/list.h>

typedef enum
{
	EXOS_THREAD_READY = 0,
	EXOS_THREAD_WAIT,
} EXOS_THREAD_STATE;

typedef struct
{
	EXOS_NODE Node;
	unsigned long SP;
	unsigned long StackStart;
	EXOS_THREAD_STATE State;
} EXOS_THREAD;


void __threads_init();
EXOS_THREAD *__kernel_schedule();

#endif  // EXOS_THREADS_H

