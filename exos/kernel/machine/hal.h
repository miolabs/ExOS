#ifndef EXOS_MACHINE_HAL_H
#define EXOS_MACHINE_HAL_H

#include <kernel/types.h>
#include <stdint.h>

#ifndef __dma
#define __dma __attribute__((__section__(".dma")))
#endif

#ifndef __naked
#define __naked __attribute__((naked)) 
#endif

extern void * const __machine_stack_start;
extern void * const __machine_stack_end;
extern void * const __machine_tbss_start;

void __machine_init();
void __machine_req_switch();
void __machine_idle();
void __machine_dsb();
void __machine_init_thread_stack(void **pstack, unsigned long arg, unsigned long pc, unsigned long lr);
void __machine_init_thread_local_storage(void **pstack);
void __machine_halt() __noreturn;

int __machine_trylock(unsigned char *lock, unsigned char value);
void __machine_unlock(unsigned char *lock);
void __machine_reset() __noreturn;

void __board_init();
void __weak __board_add_memory();
void __mem_copy(void *start, void *stop, const void *source);
void __mem_set(void *start, void *stop, unsigned char stuff_byte); 

unsigned int __str_copy(char *dst, const char *src, unsigned int max_length);
int __str_comp(const char *str1, const char *str2);
unsigned int __uint_hexlz(char *dst, unsigned int value, int tx, char tc);
unsigned int __uint_hexl(char *dst, unsigned int value);
unsigned int __int_declz(char *dst, int value, int tz, char tc);
unsigned int __int_decl(char *dst, int value);
unsigned int __uint_declz(char *dst, unsigned int value, int tz, char tc);
unsigned int __uint_decl(char *dst, unsigned int value);
unsigned int __ulong_hexlz(char *dst, unsigned long value, int tx, char tc);
unsigned int __ulong_hexl(char *dst, unsigned long value);
unsigned int __long_declz(char *dst, long value, int tz, char tc);
unsigned int __long_decl(char *dst, long value);
unsigned int __ulong_declz(char *dst, unsigned long value, int tz, char tc);
unsigned int __ulong_decl(char *dst, unsigned long value);
unsigned int __decl_uint(const char *src, unsigned int *pvalue);
unsigned int __decl_int(const char *src, int *pvalue);

#endif // EXOS_MACHINE_HAL_H
