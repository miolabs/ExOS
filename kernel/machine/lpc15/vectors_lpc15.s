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
	.word BOD_IRQHandler
	.word FLASH_IRQHandler
	.word EE_IRQHandler
	.word DMA_IRQHandler
	.word GINT0_IRQHandler
	.word GINT1_IRQHandler
	.word PIN_INT0_IRQHandler
	.word PIN_INT1_IRQHandler
	.word PIN_INT2_IRQHandler
	.word PIN_INT3_IRQHandler
	.word PIN_INT4_IRQHandler
	.word PIN_INT5_IRQHandler
	.word PIN_INT6_IRQHandler
	.word PIN_INT7_IRQHandler
	.word RIT_IRQHandler
	.word SCT0_IRQHandler
	.word SCT1_IRQHandler
	.word SCT2_IRQHandler
	.word SCT3_IRQHandler
	.word MRT_IRQHandler
	.word UART0_IRQHandler
	.word UART1_IRQHandler
	.word UART2_IRQHandler
	.word I2C0_IRQHandler
	.word SPI0_IRQHandler
	.word SPI1_IRQHandler
	.word C_CAN0_IRQHandler
	.word USB_IRQ_IRQHandler
	.word USB_FIQ_IRQHandler
	.word USBWAKEUP_IRQHandler
	.word ADC0_SEQA_IRQHandler
	.word ADC0_SEQB_IRQHandler
	.word ADC0_THCMP_IRQHandler
	.word ADC0_OVR_IRQHandler
	.word ADC1_SEQA_IRQHandler
	.word ADC1_SEQB_IRQHandler
	.word ADC1_THCMP_IRQHandler
	.word ADC1_OVR_IRQHandler
	.word DAC_IRQHandler
	.word CMP0_IRQHandler
	.word CMP1_IRQHandler
	.word CMP2_IRQHandler
	.word CMP3_IRQHandler
	.word QEI_IRQHandler
	.word RTC_ALARM_IRQHandler
	.word RTC_WAKE_IRQHandler


