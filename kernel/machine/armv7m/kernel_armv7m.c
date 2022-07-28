#include <kernel/machine/hal.h>
#include <kernel/thread.h>
#include <kernel/panic.h>
#include <kernel/syscall.h>

extern int __stack_process_start__, __stack_process_end__;
extern int __tbss_start__, __tbss_end__;
extern int __tdata_start__, __tdata_end__, __tdata_load_start__;

void * const __machine_stack_start = &__stack_process_start__;
void * const __machine_tbss_start = &__tbss_start__;

void *__switch(void *psp)
{
	ASSERT(__running_thread != NULL, KERNEL_ERROR_NULL_POINTER);
	__running_thread->SP = psp;

	// check stack limit
	ASSERT(__running_thread->SP > __running_thread->SP, KERNEL_ERROR_STACK_OVERFLOW);
	ASSERT(*((unsigned long *)__running_thread->SP) == 0xcccccccc, KERNEL_ERROR_STACK_OVERFLOW);
	
	__running_thread = __kernel_schedule();

	ASSERT(__running_thread != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(__running_thread->Node.Type == EXOS_NODE_THREAD, KERNEL_ERROR_KERNEL_PANIC);
	return __running_thread->SP;
}

__naked void PendSV_Handler()
{
	__asm__ volatile (
		"push {lr}\n\t"
		"mrs r0, psp\n\t"
		"stmdb r0!, {r4-r11}\n\t"
		"bl __switch\n\t"
		"ldmia r0!, {r4-r11}\n\t"
		"msr psp, r0\n\t"
		"pop {pc}\n\t");
}

__naked void SVC_Handler()
{
	__asm__ volatile (
		"tst lr, #4\n\t"
		"ite eq\n\t"
		"mrseq r2, msp\n\t"
		"mrsne r2, psp\n\t"
		
		"push {r2, lr}\n\t"
		"ldr r1, [r2]\n\t"		// dispatcher
		"add r0, r2, #4\n\t"	// arguments
		"blx r1\n\t"			// call dispatcher
		"pop {r2}\n\t"
	
		"str r0, [r2]\n\t"	// save return value
		"pop {pc}");
}

__naked int __kernel_do(EXOS_SYSTEM_FUNC entry, ...)
{
	__asm__ volatile (
		"push {r4, lr}\n\t"
		"mrs r4, ipsr\n\t"
		"cmp r4, #11\n\t"
		"beq 1f\n\t"
		"svc #0\n\t"
		"pop {r4, pc}\n\t"
		"1:push {r1-r3}\n\t"
		"mov r4, r0\n\t"
		"mov r0, sp\n\t"
		"blx r4\n\t"
		"pop {r1-r3}\n\t"
		"pop {r4, pc}\n\t");
}

void __machine_init_thread_stack(void **pstack, unsigned long arg, unsigned long pc, unsigned long lr)
{
	unsigned long *frame = (unsigned long *)((long)(*pstack) & ~7);

	*--frame = 0x21000000; // xPSR (C + T)
	*--frame = pc;
	*--frame = lr;
	*--frame = 12;

	*--frame = 3;
	*--frame = 2;
	*--frame = 1;
	*--frame = arg;

	*--frame = 11;
	*--frame = 10;
	*--frame = 9;
	*--frame = 8;

	*--frame = 7;
	*--frame = 6;
	*--frame = 5;
	*--frame = 4;
	
	*pstack = frame;
}

void __machine_init_thread_local_storage(void **pstack)
{
	int bss_size = &__tbss_end__ - &__tbss_start__; 
	int data_size = &__tdata_end__ - &__tdata_start__;

	void *stack_end = *pstack;

	__mem_copy(stack_end - data_size, stack_end, &__tdata_load_start__);
	stack_end -= data_size;
	__mem_set(stack_end - bss_size, stack_end, 0); 
	stack_end -= bss_size;

	*pstack = stack_end; 
}

#pragma GCC optimize(2)

unsigned char *__aeabi_read_tp(void)
{
	exos_thread_t *thread = __running_thread;
#ifdef DEBUG
	if (thread->TP == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	return thread->TP;
}


