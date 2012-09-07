#ifndef EXOS_MACHINE_HAL_H
#define EXOS_MACHINE_HAL_H

#include <kernel/types.h>

void __machine_init();
void __machine_req_switch();

void __mem_copy(void *start, void *stop, void *source);
void __mem_set(void *start, void *stop, unsigned char stuff_byte); 

#endif // EXOS_MACHINE_HAL_H
