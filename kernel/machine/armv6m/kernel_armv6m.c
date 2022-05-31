#include <kernel/machine/hal.h>
#include <kernel/thread.h>
#include <kernel/panic.h>
#include <kernel/syscall.h>

extern unsigned char __tbss_start__[], __tbss_end__[];
extern unsigned char __tdata_start__[], __tdata_end__[], __tdata_load_start__[];

__naked void PendSV_Handler()
{
	register void *psp __asm__("r0");

	// save high registers in process stack
	__asm__ volatile (
		"push {lr}\n\t"
		"mrs %0, psp\n\t"
		
		"sub %0, #32\n\t"
		"stmia %0!, {r4-r7}\n\t"
		"mov r1, r8\n\t"
		"str r1, [%0, #0]\n\t"
		"mov r1, r9\n\t"
		"str r1, [%0, #4]\n\t"
		"mov r1, r10\n\t"
		"str r1, [%0, #8]\n\t"
		"mov r1, r11\n\t"
		"str r1, [%0, #12]\n\t"
		"sub %0, #16\n\t"
		: "=r" (psp));

	if (__running_thread == NULL)
		__kernel_panic();
	
	__running_thread->SP = psp;

	__running_thread = __kernel_schedule();

#ifdef DEBUG
	if (__running_thread == NULL)
		__kernel_panic();
#endif

	psp = __running_thread->SP;
	// restore high registers from process stack
	__asm__ volatile (
		"ldmia %0!, {r4-r7}\n\t"
		"ldr r1, [%0, #0]\n\t"
		"mov r8, r1\n\t"
		"ldr r1, [%0, #4]\n\t"
		"mov r9, r1\n\t"
		"ldr r1, [%0, #8]\n\t"
		"mov r10, r1\n\t"
		"ldr r1, [%0, #12]\n\t"
		"mov r11, r1\n\t"
		"add %0, #16\n\t"
		"msr psp, %0\n\t"
		"pop {pc}\n\t"
		: : "r" (psp));
}

__naked void SVC_Handler()
{
	__asm__ volatile (
		"mov r2, #4\n\t"
		"mov r3, lr\n\t"
		"tst r3, r2\n\t"
		"bne 1f\n\t"
		"mrs r2, msp\n\t"
		"b 2f\n\t"
		"1: mrs r2, psp\n\t"

		"2: push {r2, lr}\n\t"
		"ldr r1, [r2]\n\t"		// dispatcher
		"add r0, r2, #4\n\t"	// arguments
		"blx r1\n\t"			// call dispatcher
		"pop {r2}\n\t"
	
		"str r0, [r2]\n\t"	// save return value
		"pop {pc}");
}

__naked int __kernel_do(EXOS_SYSTEM_FUNC entry, ...)
{
#ifdef DEBUG
	__asm__ volatile (
		"push {r4, lr}\n\t"
		"mrs r4, ipsr\n\t"
		"cmp r4, #11\n\t"
		"beq 1f\n\t"
		"svc #0\n\t"
		"pop {r4, pc}\n\t"
		"1: bl __kernel_panic\n\t"
		"pop {r4, pc}\n\t");
#else
	__asm__ volatile (
		"push {lr}\n\t"
		"svc #0\n\t"
		"pop {pc}");
#endif
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
	int bss_size = __tbss_end__ - __tbss_start__; 
	int data_size = __tdata_end__ - __tdata_start__;

	void *stack_end = *pstack;

	__mem_copy(stack_end - data_size, stack_end, __tdata_load_start__);
	stack_end -= data_size;
	__mem_set(stack_end - bss_size, stack_end, 0); 
	stack_end -= bss_size;

	*pstack = stack_end; 
}

#pragma GCC optimize(2)

unsigned char *__aeabi_read_tp(void)
{
	EXOS_THREAD *thread = __running_thread;
#ifdef DEBUG
	if (thread->TP == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	return thread->TP;
}

void __machine_idle()
{
	while(1)
	{
		__asm__ volatile (
			"wfi\n\t");	
	}
}

