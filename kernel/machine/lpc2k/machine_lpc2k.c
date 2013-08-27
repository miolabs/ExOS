#include <kernel/machine/hal.h>
#include <kernel/startup.h>
#include <kernel/thread.h>
#include <kernel/panic.h>
#include <kernel/syscall.h>
#include <support/board_hal.h>
#include <support/lpc2k/cpu.h>

#if OSCILLATOR_CLOCK_FREQUENCY == 12000000

// Fosc = 12Mhz, Fcco = 288Mhz
#ifndef PLLCFG_VAL
#define PLLCFG_VAL ((11 << PLLCFG_MSEL_BIT) | (0 << PLLCFG_NSEL_BIT))
#endif
#ifndef CCLKCFG_VAL
#define CCLKCFG_VAL 3 // CClk = 72 MHz
#endif
#ifndef USBCLKCFG_VAL
#define USBCLKCFG_VAL 5
#endif

#endif

volatile unsigned char __pend_switch = 0;
unsigned long __psp;

extern unsigned long __stack_start__;
const void *__machine_process_start = &__stack_start__;
extern int __data_start__, __data_end__, __data_load_start__;
extern int __bss_start__, __bss_end__;

static int _memtest(void *base, int size);

void __machine_init()
{
	// initialize data sections
	__mem_copy(&__data_start__, &__data_end__, &__data_load_start__);
	// initialize bss sections
	__mem_set(&__bss_start__, &__bss_end__, 0);

#if OSCILLATOR_CLOCK_FREQUENCY==12000000
	SystemCoreClock = 72000000;
#endif

	LPC_SC->PCONP = 0;		// Disable all peripheral power
	LPC_VIC->IntEnClr = -1; // Disable all interrupts
	hal_board_initialize();

	// enable interrupts
	__asm__ volatile (
		"mrs r0, cpsr\n\t"
		"bic r0, #0xC0\n\t"		// clear I, F bits
		"msr cpsr, r0\n\t");
}

void inline __machine_req_switch()
{
	__pend_switch = 1;
}

void __machine_switch()
{
	if (__running_thread == NULL)
		__kernel_panic();

	__running_thread->SP = (void *)__psp;

	__running_thread = __kernel_schedule();

#ifdef DEBUG
	if (__running_thread == NULL)
		__kernel_panic();
#endif

	__psp = (unsigned long)__running_thread->SP;
}



void *__machine_init_thread_stack(void *stack_end, unsigned long arg, unsigned long pc, unsigned long lr)
{
	unsigned long *frame = (unsigned long *)stack_end;
	*--frame = lr;
	*--frame = 12;
	*--frame = 0x10;	// cpsr (user mode)
	*--frame = pc;

	*--frame = 11;
	*--frame = 10;
	*--frame = 9;
	*--frame = 8;

	*--frame = 7;
	*--frame = 6;
	*--frame = 5;
	*--frame = 4;

	*--frame = 3;
	*--frame = 2;
	*--frame = 1;
	*--frame = arg;

	return frame;
}

__attribute__((naked))
int __machine_trylock(unsigned char *lock, unsigned char value)
{
	__asm__ volatile (
		"mov r2, r0\n\t"
		"swpb r0, r1, [r2]\n\t"
		"ands r0, #1\n\t"
		"moveq r0, #1\n\t"
		"movne r0, #0\n\t"
		"bx lr\n\t");
}

void __machine_unlock(unsigned char *lock)
{
	*lock = 0;
}

static int _memtest(void *base, int size)
{
	unsigned int i;
	// 32 bits access
	for (i = 0; i < (size >> 2); i++)
	{
		((unsigned int *)base)[i] = i;
	}
	i = 0;
	for (i = 0; i < (size >> 2); i++)
	{
		if (((unsigned int *)base)[i] != i)
		{
			return -1;
		}
	}

	// 16 bits access
	for (i = 0; i < 0x8000; i++)
	{
		((unsigned short *)base)[i] = i % 137;
	}
	for (i = 0; i < 0x8000; i++)
	{
		if (((unsigned short *)base)[i] != i % 137)
		{
			return -2;
		}
	}
	// 8 bits access
	for ( i = 0; i < 0x100; i++)
	{
		((unsigned char *)base)[i] = i % 13;
	}
	for (i = 0; i < 0x100; i++)
	{
		if (((unsigned char *)base)[i] != i % 13)
		{
			return -3;
		}
	}
	return 0;
}


