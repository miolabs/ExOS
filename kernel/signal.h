#ifndef EXOS_SIGNAL_H
#define EXOS_SIGNAL_H

#include <kernel/thread.h>

typedef enum
{
	EXOS_SIGB_NONE = -1,
	EXOS_SIGB_ABORT = 0,
} EXOS_SIGNAL;

#define EXOS_SIGF_ABORT (1 << EXOS_SIGB_ABORT)
#define EXOS_SIGF_RESERVED_MASK (EXOS_SIGF_ABORT)

typedef void (* EXOS_SIGNAL_HANDLER)(EXOS_SIGNAL signal);
typedef EXOS_SIGNAL_HANDLER EXOS_SIGNAL_HANDLER_TABLE[32];

#define EXOS_TIMEOUT_NEVER 0

void __signal_set(EXOS_THREAD *thread, unsigned long mask);
unsigned long __signal_wait(unsigned long mask);
EXOS_SIGNAL __signal_alloc();
void __signal_free(EXOS_THREAD *thread, EXOS_SIGNAL signal);

int exos_signal_alloc();
void exos_signal_free(EXOS_SIGNAL signal);
void exos_signal_set(EXOS_THREAD *thread, unsigned long mask);
unsigned long exos_signal_wait(unsigned long mask, unsigned long timeout);


typedef enum
{
	EXOS_WAIT_PENDING,
	EXOS_WAIT_ABORTED,
	EXOS_WAIT_DONE,
} EXOS_WAIT_STATE;

typedef struct
{
	EXOS_NODE Node;
	EXOS_THREAD *Owner;
	EXOS_SIGNAL Signal;
	EXOS_WAIT_STATE State;
} EXOS_WAIT_HANDLE;

void __cond_add_wait_handle(EXOS_LIST *list, EXOS_WAIT_HANDLE *handle);
void __cond_rem_wait_handle(EXOS_WAIT_HANDLE *handle, EXOS_WAIT_STATE state);
int __cond_signal_all(EXOS_LIST *handles);
int __cond_signal_one(EXOS_LIST *handles);
void exos_cond_abort(EXOS_WAIT_HANDLE *handle);

#endif // EXOS_SIGNAL_H