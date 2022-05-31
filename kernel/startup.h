#ifndef EXOS_STARTUP_H
#define EXOS_STARTUP_H

#include <kernel/types.h>

// prototypes
void __kernel_start();

__weak void __posix_init();

#endif // EXOS_STARTUP_H
