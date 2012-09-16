#include <CMSIS/system_LPC17xx.h>
#include <kernel/startup.h>
#include <kernel/machine/hal.h>

#define __init __attribute__((section(".init")))
#define __naked __attribute__((naked))

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
void __weak TIMER0_IRQHandler();  
void __weak TIMER1_IRQHandler();  
void __weak TIMER2_IRQHandler();  
void __weak TIMER3_IRQHandler();  
void __weak UART0_IRQHandler();   
void __weak UART1_IRQHandler();   
void __weak UART2_IRQHandler();   
void __weak UART3_IRQHandler();   
void __weak PWM1_IRQHandler();    
void __weak I2C0_IRQHandler();       
void __weak I2C1_IRQHandler();       
void __weak I2C2_IRQHandler();       
void __weak SPI_IRQHandler();        
void __weak SSP0_IRQHandler();       
void __weak SSP1_IRQHandler();       
void __weak PLL0_IRQHandler();    
void __weak RTC_IRQHandler();     
void __weak EINT0_IRQHandler();   
void __weak EINT1_IRQHandler();   
void __weak EINT2_IRQHandler();   
void __weak EINT3_IRQHandler();   
void __weak ADC_IRQHandler();     
void __weak BOD_IRQHandler();     
void __weak USB_IRQHandler();     
void __weak CAN_IRQHandler();     
void __weak DMA_IRQHandler();     
void __weak I2S_IRQHandler();     
void __weak ENET_IRQHandler();    
void __weak RIT_IRQHandler();     
void __weak MCPWM_IRQHandler();   
void __weak QEI_IRQHandler();     
void __weak PLL1_IRQHandler();    
void __weak USBActivity_IRQHandler();
void __weak CANActivity_IRQHandler();

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

#pragma weak TIMER0_IRQHandler = Default_Handler     
#pragma weak TIMER1_IRQHandler = Default_Handler     
#pragma weak TIMER2_IRQHandler = Default_Handler     
#pragma weak TIMER3_IRQHandler = Default_Handler     
#pragma weak UART0_IRQHandler = Default_Handler      
#pragma weak UART1_IRQHandler = Default_Handler      
#pragma weak UART2_IRQHandler = Default_Handler      
#pragma weak UART3_IRQHandler = Default_Handler      
#pragma weak PWM1_IRQHandler = Default_Handler       
#pragma weak I2C0_IRQHandler = Default_Handler       
#pragma weak I2C1_IRQHandler = Default_Handler       
#pragma weak I2C2_IRQHandler = Default_Handler       
#pragma weak SPI_IRQHandler = Default_Handler        
#pragma weak SSP0_IRQHandler = Default_Handler       
#pragma weak SSP1_IRQHandler = Default_Handler       
#pragma weak PLL0_IRQHandler = Default_Handler       
#pragma weak RTC_IRQHandler = Default_Handler        
#pragma weak EINT0_IRQHandler = Default_Handler      
#pragma weak EINT1_IRQHandler = Default_Handler      
#pragma weak EINT2_IRQHandler = Default_Handler      
#pragma weak EINT3_IRQHandler = Default_Handler      
#pragma weak ADC_IRQHandler = Default_Handler        
#pragma weak BOD_IRQHandler = Default_Handler        
#pragma weak USB_IRQHandler = Default_Handler        
#pragma weak CAN_IRQHandler = Default_Handler        
#pragma weak GPDMA_IRQHandler = Default_Handler        
#pragma weak I2S_IRQHandler = Default_Handler        
#pragma weak ENET_IRQHandler = Default_Handler       
#pragma weak RIT_IRQHandler = Default_Handler        
#pragma weak MCPWM_IRQHandler = Default_Handler      
#pragma weak QEI_IRQHandler = Default_Handler        
#pragma weak PLL1_IRQHandler = Default_Handler      
#pragma weak USBACT_IRQHandler = Default_Handler        
#pragma weak CANACT_IRQHandler = Default_Handler 

extern int __stack_start__, __stack_end__;
extern int __stack_process_start__, __stack_process_end__;
extern int __data_load_start__;
extern int __data_start__, __data_end__;
extern int __bss_start__, __bss_end__;

__init __naked void Reset_Handler() 
{
#ifdef DEBUG
	// initialize top of system stack
	*((unsigned long *)&__stack_start__) = 0xcccccccc;

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
	while(1);
}


