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
	.word WAKEUP_IRQHandler
	.word WAKEUP_IRQHandler
	.word WAKEUP_IRQHandler
	.word WAKEUP_IRQHandler
	.word WAKEUP_IRQHandler
	.word WAKEUP_IRQHandler
	.word WAKEUP_IRQHandler
	.word WAKEUP_IRQHandler
	.word WAKEUP_IRQHandler
	.word WAKEUP_IRQHandler
	.word WAKEUP_IRQHandler
	.word WAKEUP_IRQHandler
	.word WAKEUP_IRQHandler
	.word CAN_IRQHandler
	.word SSP1_IRQHandler
	.word I2C_IRQHandler
	.word TIMER16_0_IRQHandler
	.word TIMER16_1_IRQHandler
	.word TIMER32_0_IRQHandler
	.word TIMER32_1_IRQHandler
	.word SSP0_IRQHandler
	.word UART_IRQHandler
	.word 0 // Reserved
	.word 0 // Reserved
	.word ADC_IRQHandler
	.word WDT_IRQHandler
	.word BOD_IRQHandler
	.word 0 // Reserved
	.word PIOINT3_IRQHandler
	.word PIOINT2_IRQHandler
	.word PIOINT1_IRQHandler
	.word PIOINT0_IRQHandler 





