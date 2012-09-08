#ifndef EXOS_STARTUP_H
#define EXOS_STARTUP_H

#include <kernel/types.h>

<<<<<<< HEAD
// prototypes
void __kernel_start();

=======



void __kernel_start();

// external functions
void __machine_init();
void __mem_copy(void *start, void *stop, void *source);
void __mem_set(void *start, void *stop, unsigned char stuff_byte); 

>>>>>>> 5c314a936eb4b82d55df2ee2c11c7f12fc824acb
#endif // EXOS_STARTUP_H
