#include <kernel/startup.h>
#include <kernel/threads.h>
#include <kernel/panic.h>
#include <kernel/syscall.h>
#include <CMSIS/LPC17xx.h>

extern EXOS_THREAD *__running_thread;

void __machine_init()
{
	

	// set lowest priority for PendSV
	NVIC_SetPriority(PendSV_IRQn, 0xFF);	

	for (int i = 0; i <= 34; i++)
	{
		// set lowest priority for IRQ
		NVIC_SetPriority((IRQn_Type)i, 0xFF);
	}
}

void __machine_req_switch()
{
	SCB->ICSR = (1 << 28);	// Set pending PendSV service (switch)
}

#define __naked __attribute__((naked)) 

__naked void PendSV_Handler()
{
	register unsigned long psp __asm__("r0");

	// save high registers in process stack
	__asm__ volatile (
		"MRS %0, psp\n\t"
		"STMDB %0!, {r4-r11}\n\t"
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
		"LDMIA %0!, {r4-r11}\n\t"
		"MSR psp, %0\n\t"
		: : "r" (psp));
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
#ifdef DEBUG
	__asm__ volatile (
		"push {r4, lr}\n\t"
		"mrs r4, ipsr\n\t"
		"cmp r4, #11\n\t"
		"beq __kernel_panic\n\t"
		"svc #0\n\t"
		"pop {r4, pc}");
#else
	__asm__ volatile (
		"push {lr}\n\t"
		"svc #0\n\t"
		"pop {pc}");
#endif
}
