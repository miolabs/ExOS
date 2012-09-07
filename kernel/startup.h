#ifndef EXOS_STARTUP_H
#define EXOS_STARTUP_H

#include <kernel/types.h>




void __kernel_start();

// external functions
void __machine_init();
void __mem_copy(void *start, void *stop, void *source);
void __mem_set(void *start, void *stop, unsigned char stuff_byte); 

#endif // EXOS_STARTUP_H
