#include <CMSIS/nrf51.h>
#include <kernel/startup.h>
#include <kernel/machine/hal.h>

#define __init __attribute__((section(".init")))
#define __naked __attribute__((naked))

extern int __stack_start__, __stack_end__;
extern int __stack_process_start__, __stack_process_end__;
extern int __data_start__, __data_end__, __data_load_start__;
extern int __bss_start__, __bss_end__;
#ifdef __RAM2_ENABLE
extern int __data2_start__, __data2_end__, __data2_load_start__;
extern int __bss2_start__, __bss2_end__;
#endif

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

	// initialize data sections and bss
	__mem_copy(&__data_start__, &__data_end__, &__data_load_start__);
	__mem_set(&__bss_start__, &__bss_end__, 0);
#ifdef __RAM2_ENABLE
	__mem_copy(&__data2_start__, &__data2_end__, &__data2_load_start__);
	__mem_set(&__bss2_start__, &__bss2_end__, 0);
#endif

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

void __weak POWER_CLOCK_IRQHandler();
void __weak RADIO_IRQHandler();
void __weak UART0_IRQHandler();
void __weak SPI0_TWI0_IRQHandler();
void __weak SPI1_TWI1_IRQHandler();
void __weak GPIOTE_IRQHandler();
void __weak ADC_IRQHandler();
void __weak TIMER0_IRQHandler();
void __weak TIMER1_IRQHandler();
void __weak TIMER2_IRQHandler();
void __weak RTC0_IRQHandler();
void __weak TEMP_IRQHandler();
void __weak RNG_IRQHandler();
void __weak ECB_IRQHandler();
void __weak CCM_AAR_IRQHandler();
void __weak WDT_IRQHandler();
void __weak RTC1_IRQHandler();
void __weak QDEC_IRQHandler();
void __weak LPCOMP_COMP_IRQHandler();
void __weak SWI0_IRQHandler();
void __weak SWI1_IRQHandler();
void __weak SWI2_IRQHandler();
void __weak SWI3_IRQHandler();
void __weak SWI4_IRQHandler();
void __weak SWI5_IRQHandler();


#pragma weak NMI_Handler = Default_Handler
#pragma weak HardFault_Handler = Default_Handler
#pragma weak MemManage_Handler = Default_Handler
#pragma weak BusFault_Handler = Default_Handler
#pragma weak UsageFault_Handler = Default_Handler
#pragma weak SVC_Handler = Default_Handler
#pragma weak DebugMon_Handler = Default_Handler
#pragma weak PendSV_Handler = Default_Handler
#pragma weak SysTick_Handler = Default_Handler

#pragma weak POWER_CLOCK_IRQHandler = Default_Handler
#pragma weak RADIO_IRQHandler = Default_Handler
#pragma weak UART0_IRQHandler = Default_Handler
#pragma weak SPI0_TWI0_IRQHandler = Default_Handler
#pragma weak SPI1_TWI1_IRQHandler = Default_Handler
#pragma weak GPIOTE_IRQHandler = Default_Handler
#pragma weak ADC_IRQHandler = Default_Handler
#pragma weak TIMER0_IRQHandler = Default_Handler
#pragma weak TIMER1_IRQHandler = Default_Handler
#pragma weak TIMER2_IRQHandler = Default_Handler
#pragma weak RTC0_IRQHandler = Default_Handler
#pragma weak TEMP_IRQHandler = Default_Handler
#pragma weak RNG_IRQHandler = Default_Handler
#pragma weak ECB_IRQHandler = Default_Handler
#pragma weak CCM_AAR_IRQHandler = Default_Handler
#pragma weak WDT_IRQHandler = Default_Handler
#pragma weak RTC1_IRQHandler = Default_Handler
#pragma weak QDEC_IRQHandler = Default_Handler
#pragma weak LPCOMP_COMP_IRQHandler = Default_Handler
#pragma weak SWI0_IRQHandler = Default_Handler
#pragma weak SWI1_IRQHandler = Default_Handler
#pragma weak SWI2_IRQHandler = Default_Handler
#pragma weak SWI3_IRQHandler = Default_Handler
#pragma weak SWI4_IRQHandler = Default_Handler
#pragma weak SWI5_IRQHandler = Default_Handler

