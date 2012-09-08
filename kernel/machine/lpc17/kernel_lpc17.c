#include <kernel/startup.h>
#include <kernel/threads.h>
#include <kernel/panic.h>
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

