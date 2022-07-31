#include <NUC1xx.h>
#include <kernel/startup.h>
#include <kernel/machine/hal.h>

#define __init __attribute__((section(".init")))
#define __naked __attribute__((naked))

extern int __stack_start__, __stack_end__;
extern int __stack_process_start__, __stack_process_end__;
extern int __data_start__, __data_end__, __data_load_start__;
//extern int __data2_start__, __data2_end__, __data2_load_start__;
extern int __bss_start__, __bss_end__;
//extern int __bss2_start__, __bss2_end__;

const void *__machine_process_start = &__stack_process_start__;

__init __naked void Reset_Handler() 
{
	__disable_irq();

#ifdef DEBUG
	// initialize process stack
	__mem_set(&__stack_process_start__, &__stack_process_end__, 0xcc);
#endif

	// switch to process stack
	void *psp = &__stack_process_end__;
	__asm__ volatile (
		"msr psp, %0\n\t"
		"mov %0, #2\n\t"
		"msr control, %0"
		: : "r" (psp));

#ifdef DEBUG
	// initialize system stack
	__mem_set(&__stack_start__, &__stack_end__, 0xcc);
#endif

	// initialize data sections
	__mem_copy(&__data_start__, &__data_end__, &__data_load_start__);
//	__mem_copy(&__data2_start__, &__data2_end__, &__data2_load_start__);
	// initialize bss sections
	__mem_set(&__bss_start__, &__bss_end__, 0);
//	__mem_set(&__bss2_start__, &__bss2_end__, 0);

	SystemInit();
	
	__kernel_start();

	while(1);
}

__init __naked void Default_Handler()
{
	while(1);
}

// weak IRQ handler prototypes
void __weak Reset_Handler();
void __weak NMI_Handler();
void __weak HardFault_Handler();
void __weak MemManage_Handler();
void __weak BusFault_Handler();
void __weak UsageFault_Handler();
void __weak SVC_Handler();
void __weak DebugMon_Handler();
void __weak PendSV_Handler();
void __weak SysTick_Handler();

void __weak BOD_IRQHandler();
void __weak WDT_IRQHandler();
void __weak EINT0_IRQHandler();
void __weak EINT1_IRQHandler();
void __weak GPAB_IRQHandler();
void __weak GPCDE_IRQHandler();
void __weak PWMA_IRQHandler();
void __weak PWMB_IRQHandler();
void __weak TMR0_IRQHandler();
void __weak TMR1_IRQHandler();
void __weak TMR2_IRQHandler();
void __weak TMR3_IRQHandler();
void __weak UART02_IRQHandler();
void __weak UART1_IRQHandler();
void __weak SPI0_IRQHandler();
void __weak SPI1_IRQHandler();
void __weak SPI2_IRQHandler();
void __weak SPI3_IRQHandler();
void __weak I2C0_IRQHandler();
void __weak I2C1_IRQHandler();
void __weak CAN0_IRQHandler();
void __weak USBD_IRQHandler();
void __weak PS2_IRQHandler();
void __weak ACMP_IRQHandler();
void __weak PDMA_IRQHandler();
void __weak I2S_IRQHandler();
void __weak PWRWU_IRQHandler();
void __weak ADC_IRQHandler();
void __weak RTC_IRQHandler();


#pragma weak NMI_Handler = Default_Handler
#pragma weak HardFault_Handler = Default_Handler
#pragma weak MemManage_Handler = Default_Handler
#pragma weak BusFault_Handler = Default_Handler
#pragma weak UsageFault_Handler = Default_Handler
#pragma weak SVC_Handler = Default_Handler
#pragma weak DebugMon_Handler = Default_Handler
#pragma weak PendSV_Handler = Default_Handler
#pragma weak SysTick_Handler = Default_Handler

#pragma weak BOD_IRQHandler = Default_Handler
#pragma weak WDT_IRQHandler = Default_Handler
#pragma weak EINT0_IRQHandler = Default_Handler
#pragma weak EINT1_IRQHandler = Default_Handler
#pragma weak GPAB_IRQHandler = Default_Handler
#pragma weak GPCDE_IRQHandler = Default_Handler
#pragma weak PWMA_IRQHandler = Default_Handler
#pragma weak PWMB_IRQHandler = Default_Handler
#pragma weak TMR0_IRQHandler = Default_Handler
#pragma weak TMR1_IRQHandler = Default_Handler
#pragma weak TMR2_IRQHandler = Default_Handler
#pragma weak TMR3_IRQHandler = Default_Handler
#pragma weak UART02_IRQHandler = Default_Handler
#pragma weak UART1_IRQHandler = Default_Handler
#pragma weak SPI0_IRQHandler = Default_Handler
#pragma weak SPI1_IRQHandler = Default_Handler
#pragma weak SPI2_IRQHandler = Default_Handler
#pragma weak SPI3_IRQHandler = Default_Handler
#pragma weak I2C0_IRQHandler = Default_Handler
#pragma weak I2C1_IRQHandler = Default_Handler
#pragma weak CAN0_IRQHandler = Default_Handler
#pragma weak USBD_IRQHandler = Default_Handler
#pragma weak PS2_IRQHandler = Default_Handler
#pragma weak ACMP_IRQHandler = Default_Handler
#pragma weak PDMA_IRQHandler = Default_Handler
#pragma weak I2S_IRQHandler = Default_Handler
#pragma weak PWRWU_IRQHandler = Default_Handler
#pragma weak ADC_IRQHandler = Default_Handler
#pragma weak RTC_IRQHandler = Default_Handler

