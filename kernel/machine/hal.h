#ifndef EXOS_MACHINE_HAL_H
#define EXOS_MACHINE_HAL_H

#include <kernel/types.h>

extern const void *__machine_process_start;

void __machine_init();
void __machine_req_switch();
void __machine_idle();
void *__machine_init_thread_stack(void *stack_end, unsigned long arg, unsigned long pc, unsigned long lr);

void __mem_copy(void *start, void *stop, const void *source);
void __mem_set(void *start, void *stop, unsigned char stuff_byte); 

unsigned int __str_copy(char *dst, const char *src, unsigned int max_length);
int __str_comp(const char *str1, const char *str2);
unsigned int __uint32_hexl(char *dst, unsigned long v);

#endif // EXOS_MACHINE_HAL_H
