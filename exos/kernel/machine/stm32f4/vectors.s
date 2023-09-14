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
#ifdef RESVD34_VECTOR
        .word RESVD34_VECTOR
#else
	.word 0 // Reserved
#endif
	.word PendSV_Handler
	.word SysTick_Handler 
	.word WWDG_IRQHandler
	.word PVD_IRQHandler
	.word TAMP_STAMP_IRQHandler
	.word RTC_WKUP_IRQHandler
	.word FLASH_IRQHandler
	.word RCC_IRQHandler
	.word EXTI0_IRQHandler
	.word EXTI1_IRQHandler
	.word EXTI2_IRQHandler
	.word EXTI3_IRQHandler
	.word EXTI4_IRQHandler
	.word DMA1_Stream0_IRQHandler
	.word DMA1_Stream1_IRQHandler
	.word DMA1_Stream2_IRQHandler
	.word DMA1_Stream3_IRQHandler
	.word DMA1_Stream4_IRQHandler
	.word DMA1_Stream5_IRQHandler
	.word DMA1_Stream6_IRQHandler
	.word ADC_IRQHandler
	.word CAN1_TX_IRQHandler
	.word CAN1_RX0_IRQHandler
	.word CAN1_RX1_IRQHandler
	.word CAN1_SCE_IRQHandler
	.word EXTI9_5_IRQHandler
	.word TIM1_BRK_TIM9_IRQHandler
	.word TIM1_UP_TIM10_IRQHandler
	.word TIM1_TRG_COM_TIM11_IRQHandler
	.word TIM1_CC_IRQHandler
	.word TIM2_IRQHandler
	.word TIM3_IRQHandler
	.word TIM4_IRQHandler
	.word I2C1_EV_IRQHandler
	.word I2C1_ER_IRQHandler
	.word I2C2_EV_IRQHandler
	.word I2C2_ER_IRQHandler
	.word SPI1_IRQHandler
	.word SPI2_IRQHandler
	.word USART1_IRQHandler
	.word USART2_IRQHandler
	.word USART3_IRQHandler
	.word EXTI15_10_IRQHandler
	.word RTC_Alarm_IRQHandler
	.word OTG_FS_WKUP_IRQHandler
	.word TIM8_BRK_TIM12_IRQHandler
	.word TIM8_UP_TIM13_IRQHandler
	.word TIM8_TRG_COM_TIM14_IRQHandler
	.word TIM8_CC_IRQHandler
	.word DMA1_Stream7_IRQHandler
	.word FSMC_IRQHandler
	.word SDIO_IRQHandler
	.word TIM5_IRQHandler
	.word SPI3_IRQHandler
	.word UART4_IRQHandler
	.word UART5_IRQHandler
	.word TIM6_DAC_IRQHandler
	.word TIM7_IRQHandler
	.word DMA2_Stream0_IRQHandler
	.word DMA2_Stream1_IRQHandler
	.word DMA2_Stream2_IRQHandler
	.word DMA2_Stream3_IRQHandler
	.word DMA2_Stream4_IRQHandler
	.word ETH_IRQHandler
	.word ETH_WKUP_IRQHandler
	.word CAN2_TX_IRQHandler
	.word CAN2_RX0_IRQHandler
	.word CAN2_RX1_IRQHandler
	.word CAN2_SCE_IRQHandler
	.word OTG_FS_IRQHandler
	.word DMA2_Stream5_IRQHandler
	.word DMA2_Stream6_IRQHandler
	.word DMA2_Stream7_IRQHandler
	.word USART6_IRQHandler
	.word I2C3_EV_IRQHandler
	.word I2C3_ER_IRQHandler
	.word OTG_HS_EP1_OUT_IRQHandler
	.word OTG_HS_EP1_IN_IRQHandler
	.word OTG_HS_WKUP_IRQHandler
	.word OTG_HS_IRQHandler
	.word DCMI_IRQHandler
	.word CRYP_IRQHandler
	.word HASH_RNG_IRQHandler
	.word FPU_IRQHandler
	.word UART7_IRQHandler
	.word UART8_IRQHandler
	.word SPI4_IRQHandler
	.word SPI5_IRQHandler
	.word SPI6_IRQHandler
	.word SAI1_IRQHandler
	.word LTDC_IRQHandler
	.word LTDC_ER_IRQHandler
	.word DMA2D_IRQHandler

	.word QUADSPI_IRQHandler
	.word DSI_IRQHandler

