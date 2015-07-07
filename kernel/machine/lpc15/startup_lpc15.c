#include <CMSIS/system_LPC15xx.h>
#include <kernel/startup.h>
#include <kernel/machine/hal.h>

#define __init __attribute__((section(".init")))
#define __naked __attribute__((naked))

extern int __stack_start__, __stack_end__;
extern int __stack_process_start__, __stack_process_end__;
extern int __data_start__, __data_end__, __data_load_start__;
extern int __bss_start__, __bss_end__;

const void *__machine_process_start = &__stack_process_start__;

__init __naked void Reset_Handler() 
{
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
	// initialize bss sections
	__mem_set(&__bss_start__, &__bss_end__, 0);

	SystemInit();
	
	__kernel_start();

	while(1);
}

__init __naked void Default_Handler()
{
	__machine_reset();
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
void __weak WDT_IRQHandler();
void __weak BOD_IRQHandler();
void __weak FLASH_IRQHandler();
void __weak EE_IRQHandler();
void __weak DMA_IRQHandler();
void __weak GINT0_IRQHandler();
void __weak GINT1_IRQHandler();
void __weak PIN_INT0_IRQHandler();
void __weak PIN_INT1_IRQHandler();
void __weak PIN_INT2_IRQHandler();
void __weak PIN_INT3_IRQHandler();
void __weak PIN_INT4_IRQHandler();
void __weak PIN_INT5_IRQHandler();
void __weak PIN_INT6_IRQHandler();
void __weak PIN_INT7_IRQHandler();
void __weak RIT_IRQHandler();
void __weak SCT0_IRQHandler();
void __weak SCT1_IRQHandler();
void __weak SCT2_IRQHandler();
void __weak SCT3_IRQHandler();
void __weak MRT_IRQHandler();
void __weak UART0_IRQHandler();
void __weak UART1_IRQHandler();
void __weak UART2_IRQHandler();
void __weak I2C0_IRQHandler();
void __weak SPI0_IRQHandler();
void __weak SPI1_IRQHandler();
void __weak C_CAN0_IRQHandler();
void __weak USB_IRQ_IRQHandler();
void __weak USB_FIQ_IRQHandler();
void __weak USBWAKEUP_IRQHandler();
void __weak ADC0_SEQA_IRQHandler();
void __weak ADC0_SEQB_IRQHandler();
void __weak ADC0_THCMP_IRQHandler();
void __weak ADC0_OVR_IRQHandler();
void __weak ADC1_SEQA_IRQHandler();
void __weak ADC1_SEQB_IRQHandler();
void __weak ADC1_THCMP_IRQHandler();
void __weak ADC1_OVR_IRQHandler();
void __weak DAC_IRQHandler();
void __weak CMP0_IRQHandler();
void __weak CMP1_IRQHandler();
void __weak CMP2_IRQHandler();
void __weak CMP3_IRQHandler();
void __weak QEI_IRQHandler();
void __weak RTC_ALARM_IRQHandler();
void __weak RTC_WAKE_IRQHandler();

#pragma weak NMI_Handler = Default_Handler
#pragma weak HardFault_Handler = Default_Handler
#pragma weak MemManage_Handler = Default_Handler
#pragma weak BusFault_Handler = Default_Handler
#pragma weak UsageFault_Handler = Default_Handler
#pragma weak SVC_Handler = Default_Handler
#pragma weak DebugMon_Handler = Default_Handler
#pragma weak PendSV_Handler = Default_Handler
#pragma weak SysTick_Handler = Default_Handler
#pragma weak WDT_IRQHandler = Default_Handler

#pragma weak BOD_IRQHandler = Default_Handler
#pragma weak FLASH_IRQHandler = Default_Handler
#pragma weak EE_IRQHandler = Default_Handler
#pragma weak DMA_IRQHandler = Default_Handler
#pragma weak GINT0_IRQHandler = Default_Handler
#pragma weak GINT1_IRQHandler = Default_Handler
#pragma weak PIN_INT0_IRQHandler = Default_Handler
#pragma weak PIN_INT1_IRQHandler = Default_Handler
#pragma weak PIN_INT2_IRQHandler = Default_Handler
#pragma weak PIN_INT3_IRQHandler = Default_Handler
#pragma weak PIN_INT4_IRQHandler = Default_Handler
#pragma weak PIN_INT5_IRQHandler = Default_Handler
#pragma weak PIN_INT6_IRQHandler = Default_Handler
#pragma weak PIN_INT7_IRQHandler = Default_Handler
#pragma weak RIT_IRQHandler = Default_Handler
#pragma weak SCT0_IRQHandler = Default_Handler
#pragma weak SCT1_IRQHandler = Default_Handler
#pragma weak SCT2_IRQHandler = Default_Handler
#pragma weak SCT3_IRQHandler = Default_Handler
#pragma weak MRT_IRQHandler = Default_Handler
#pragma weak UART0_IRQHandler = Default_Handler
#pragma weak UART1_IRQHandler = Default_Handler
#pragma weak UART2_IRQHandler = Default_Handler
#pragma weak I2C0_IRQHandler = Default_Handler
#pragma weak SPI0_IRQHandler = Default_Handler
#pragma weak SPI1_IRQHandler = Default_Handler
#pragma weak C_CAN0_IRQHandler = Default_Handler
#pragma weak USB_IRQ_IRQHandler = Default_Handler
#pragma weak USB_FIQ_IRQHandler = Default_Handler
#pragma weak USBWAKEUP_IRQHandler = Default_Handler
#pragma weak ADC0_SEQA_IRQHandler = Default_Handler
#pragma weak ADC0_SEQB_IRQHandler = Default_Handler
#pragma weak ADC0_THCMP_IRQHandler = Default_Handler
#pragma weak ADC0_OVR_IRQHandler = Default_Handler
#pragma weak ADC1_SEQA_IRQHandler = Default_Handler
#pragma weak ADC1_SEQB_IRQHandler = Default_Handler
#pragma weak ADC1_THCMP_IRQHandler = Default_Handler
#pragma weak ADC1_OVR_IRQHandler = Default_Handler
#pragma weak DAC_IRQHandler = Default_Handler
#pragma weak CMP0_IRQHandler = Default_Handler
#pragma weak CMP1_IRQHandler = Default_Handler
#pragma weak CMP2_IRQHandler = Default_Handler
#pragma weak CMP3_IRQHandler = Default_Handler
#pragma weak QEI_IRQHandler = Default_Handler
#pragma weak RTC_ALARM_IRQHandler = Default_Handler
#pragma weak RTC_WAKE_IRQHandler = Default_Handler

