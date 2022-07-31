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
	.word POWER_CLOCK_IRQHandler
	.word RADIO_IRQHandler
	.word UART0_IRQHandler
	.word SPI0_TWI0_IRQHandler
	.word SPI1_TWI1_IRQHandler
	.word 0 // Reserved
	.word GPIOTE_IRQHandler
	.word ADC_IRQHandler
	.word TIMER0_IRQHandler
	.word TIMER1_IRQHandler
	.word TIMER2_IRQHandler
	.word RTC0_IRQHandler
	.word TEMP_IRQHandler
	.word RNG_IRQHandler
	.word ECB_IRQHandler
	.word CCM_AAR_IRQHandler
	.word WDT_IRQHandler
	.word RTC1_IRQHandler
	.word QDEC_IRQHandler
	.word LPCOMP_COMP_IRQHandler
	.word SWI0_IRQHandler
	.word SWI1_IRQHandler
	.word SWI2_IRQHandler
	.word SWI3_IRQHandler
	.word SWI4_IRQHandler
	.word SWI5_IRQHandler
	.word 0 // Reserved
	.word 0 // Reserved
	.word 0 // Reserved
	.word 0 // Reserved
	.word 0 // Reserved
	.word 0 // Reserved





