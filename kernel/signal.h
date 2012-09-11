#ifndef EXOS_SIGNAL_H
#define EXOS_SIGNAL_H

#include <kernel/thread.h>

typedef enum
{
	EXOS_SIGB_KILL = 0,
	// TODO: define reserved signals
	EXOS_SIGB_RESERVED_COUNT,
} EXOS_SIGNAL;

#define EXOS_SIGF_FILL (1 << EXOS_SIGB_KILL)
#define EXOS_SIGF_RESERVED_MASK ((1 << EXOS_SIGB_RESERVED_COUNT) - 1)

typedef void (* EXOS_SIGNAL_HANDLER)(EXOS_SIGNAL signal);
typedef EXOS_SIGNAL_HANDLER EXOS_SIGNAL_HANDLER_TABLE[32];

void __signal_set(EXOS_THREAD *thread, unsigned long mask);
int __signal_alloc();
void __signal_free(EXOS_THREAD *thread, int signal);

int exos_signal_alloc();
void exos_signal_free(int signal);
void exos_signal_set(EXOS_THREAD *thread, unsigned long mask);
unsigned long exos_signal_wait(unsigned long mask);

#endif // EXOS_SIGNAL_H