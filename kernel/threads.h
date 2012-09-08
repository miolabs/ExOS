#ifndef EXOS_THREADS_H
#define EXOS_THREADS_H

#include <kernel/list.h>

<<<<<<< HEAD
typedef void (* THREAD_FUNC)();

=======
>>>>>>> 5c314a936eb4b82d55df2ee2c11c7f12fc824acb
typedef enum
{
	EXOS_THREAD_READY = 0,
	EXOS_THREAD_WAIT,
<<<<<<< HEAD
	EXOS_THREAD_FINISHED,
=======
>>>>>>> 5c314a936eb4b82d55df2ee2c11c7f12fc824acb
} EXOS_THREAD_STATE;

typedef struct
{
	EXOS_NODE Node;
<<<<<<< HEAD
	EXOS_THREAD_STATE State;
	void *SP;
	void *StackStart;
} EXOS_THREAD;

void __threads_init();
EXOS_THREAD *__kernel_schedule();
void threads_create(EXOS_THREAD *thread, int pri, void *stack, int stack_size, THREAD_FUNC entry, void *arg);
void threads_join();
void threads_set_pri(int pri);
=======
	unsigned long SP;
	unsigned long StackStart;
	EXOS_THREAD_STATE State;
} EXOS_THREAD;


void __threads_init();
EXOS_THREAD *__kernel_schedule();

// external functions
void __machine_req_switch();
>>>>>>> 5c314a936eb4b82d55df2ee2c11c7f12fc824acb

#endif  // EXOS_THREADS_H

