	.section .vectors, "a"
	.global _vectors

_vectors:
	.word __stack_end__
	.word Reset_Handler
	.word NMI_Handler
	.word HardFault_Handler
	.word MemManage_Handler
	.word BusFault_Handler
	.word UsageFault_Handler
	.word 0 // Reserved
	.word 0 // Reserved
	.word 0 // Reserved
	.word 0 // Reserved
	.word SVC_Handler
	.word DebugMon_Handler
	.word 0 // Reserved
	.word PendSV_Handler
	.word SysTick_Handler 
	.word WDT_IRQHandler
	.word TIMER0_IRQHandler
	.word TIMER1_IRQHandler
	.word TIMER2_IRQHandler
	.word TIMER3_IRQHandler
	.word UART0_IRQHandler
	.word UART1_IRQHandler
	.word UART2_IRQHandler
	.word UART3_IRQHandler
	.word PWM1_IRQHandler
	.word I2C0_IRQHandler
	.word I2C1_IRQHandler
	.word I2C2_IRQHandler
	.word SPI_IRQHandler
	.word SSP0_IRQHandler
	.word SSP1_IRQHandler
	.word PLL0_IRQHandler
	.word RTC_IRQHandler
	.word EINT0_IRQHandler
	.word EINT1_IRQHandler
	.word EINT2_IRQHandler
	.word EINT3_IRQHandler
	.word ADC_IRQHandler
	.word BOD_IRQHandler
	.word USB_IRQHandler
	.word CAN_IRQHandler
	.word GPDMA_IRQHandler
	.word I2S_IRQHandler
	.word ENET_IRQHandler
	.word RIT_IRQHandler
	.word MCPWM_IRQHandler
	.word QEI_IRQHandler
	.word PLL1_IRQHandler
	.word USBACT_IRQHandler
	.word CANACT_IRQHandler





