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
	.word BOD_IRQHandler
	.word WDT_IRQHandler
	.word EINT0_IRQHandler
	.word EINT1_IRQHandler
	.word GPAB_IRQHandler
	.word GPCDE_IRQHandler
	.word PWMA_IRQHandler
	.word PWMB_IRQHandler
	.word TMR0_IRQHandler
	.word TMR1_IRQHandler
	.word TMR2_IRQHandler
	.word TMR3_IRQHandler
	.word UART02_IRQHandler
	.word UART1_IRQHandler
	.word SPI0_IRQHandler
	.word SPI1_IRQHandler
	.word SPI2_IRQHandler
	.word SPI3_IRQHandler
	.word I2C0_IRQHandler
	.word I2C1_IRQHandler
	.word CAN0_IRQHandler
	.word Default_Handler
	.word Default_Handler
	.word USBD_IRQHandler
	.word PS2_IRQHandler
	.word ACMP_IRQHandler
	.word PDMA_IRQHandler
	.word I2S_IRQHandler
	.word PWRWU_IRQHandler
	.word ADC_IRQHandler
	.word Default_Handler
	.word RTC_IRQHandler





