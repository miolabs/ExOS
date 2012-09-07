#ifndef EXOS_SYSCALL_H
#define EXOS_SYSCALL_H

#include <kernel/types.h>

typedef int (* EXOS_SYSTEM_FUNC)(unsigned long *args);

int __kernel_do(EXOS_SYSTEM_FUNC entry, ...);	// machine specific

#endif // EXOS_SYSCALL_H
