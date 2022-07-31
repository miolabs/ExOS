#ifndef EXOS_THREADS_H
#define EXOS_THREADS_H

#include <kernel/list.h>
#include <stdbool.h>

typedef void *(*exos_thread_func_t)(void *args);
typedef unsigned long exos_wait_mask_t;

typedef enum
{
	EXOS_THREAD_DETACHED = 0, 
	EXOS_THREAD_READY, 
	EXOS_THREAD_WAIT
} exos_thread_state_t;

typedef struct 
{
	node_t Node;
	exos_thread_state_t State;
	void *SP;
	void *StackStart;
	unsigned long StackSize;
	void *TP;
	list_t Pending;
	exos_wait_mask_t MaskUsed;
	exos_wait_mask_t MaskWait;
	list_t Joining;
#ifdef THREAD_DEBUG
	void (*Debugger)(struct __thread_struct *thread);
#endif
} exos_thread_t;

extern exos_thread_t *__running_thread;

typedef struct 
{
	node_t Node;
	enum { WAIT_HANDLE_DETACHED = 0, WAIT_HANDLE_WAITING, WAIT_HANDLE_DONE } State;
	exos_thread_t *Owner;
	exos_wait_mask_t Mask;
} exos_wait_handle_t;

typedef list_t *(* exos_wait_cond_t)(void *state);

#define EXOS_TIMEOUT_NEVER 0

// called by kernel
extern exos_thread_t *__kernel_schedule();

void __thread_init();
void exos_thread_create(exos_thread_t *thread, int pri, void *stack, unsigned stack_size, exos_thread_func_t entry, void *arg);
void exos_thread_exit();
void *exos_thread_join(exos_thread_t *thread);

void exos_thread_set_pri(int pri);

//! Create a wait handle for current thread
void exos_thread_create_wait_handle(exos_wait_handle_t *handle);
exos_wait_mask_t exos_thread_add_wait_handle(exos_wait_handle_t *handle, exos_wait_cond_t cond, void *state);
void exos_thread_dispose_wait_handle(exos_wait_handle_t *handle);
void exos_thread_resume(exos_wait_handle_t *handle);
unsigned exos_thread_resume_all(list_t *wait_handles);

void exos_thread_wait(exos_wait_mask_t mask);
void exos_thread_sleep(unsigned ticks);

#ifdef THREAD_DEBUG
void thread_set_debug_func(exos_thread_t *thread, void (*func)(exos_thread_t *thread));
#endif

#endif  // EXOS_THREADS_H

