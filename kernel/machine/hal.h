#ifndef EXOS_MACHINE_HAL_H
#define EXOS_MACHINE_HAL_H

#include <kernel/types.h>
#include <stdint.h>

#ifndef __dma
#define __dma __attribute__((__section__(".dma")))
#endif

extern const void *__machine_process_start;
extern const void *__machine_tls_start;

void __machine_init();
void __machine_req_switch();
void __machine_idle();
void *__machine_init_thread_stack(void *stack_end, unsigned long arg, unsigned long pc, unsigned long lr);
int __machine_init_thread_local_storage(void *stack_end);

int __machine_trylock(unsigned char *lock, unsigned char value);
void __machine_unlock(unsigned char *lock);
void __machine_reset();

void __mem_copy(void *start, void *stop, const void *source);
void __mem_set(void *start, void *stop, unsigned char stuff_byte); 

unsigned int __str_copy(char *dst, const char *src, unsigned int max_length);
int __str_comp(const char *str1, const char *str2);
unsigned int __uint32_hexlz(char *dst, unsigned int value, int tx, char tc);
unsigned int __uint32_hexl(char *dst, unsigned int value);
unsigned int __int32_declz(char *dst, int value, int tz, char tc);
unsigned int __int32_decl(char *dst, int value);
unsigned int __decl_uint32(const char *src, unsigned int *pvalue);

#endif // EXOS_MACHINE_HAL_H
