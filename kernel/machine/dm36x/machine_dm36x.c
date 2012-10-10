#include <kernel/machine/hal.h>
#include <kernel/startup.h>
#include <kernel/thread.h>
#include <kernel/panic.h>
#include <kernel/syscall.h>
#include <support/board_hal.h>
#include <support/dm36x/system.h>
#include <support/dm36x/emif.h>

volatile unsigned char __pend_switch = 0;
unsigned long __psp;

extern unsigned long __stack_start__;
const void *__machine_process_start = &__stack_start__;

static int _memtest(void *base, int size);

void __machine_init()
{
	hal_board_initialize();

	// 432 MHz
	int ratios_pllc2[] = { 
		9,	// SYSCLK1 = 48 MHz (USB Ref)
		2,	// SYSCLK2 = 216 MHz (ARM9)
		18,	// SYSCLK3 = 24 MHz (-DDR2)
		27,	// SYSCLK4 = 16 MHz (VoiceCodec)
		16,	// SYSCLK5 = 27 MHz (VENC)
		}; 
	pllc_setup(PLLC2, 0, 9, 0, ratios_pllc2, 5);

	system_select_armss_clock(PLLC2);

/*	gpio_setup(35, GPIO_DIR_OUTPUT, 1);	// CLKOUT1
	gpio_setup(31, GPIO_DIR_OUTPUT, 1);	// CLKOUT2 (PLLC1_SYSCLK9 * DIV)
	system_select_pinmux(31, 3);	// for testing purposes only
*/

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


