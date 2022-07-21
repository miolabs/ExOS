#include <stm32f4xx.h>
#include <kernel/machine/hal.h>
#include <kernel/startup.h>

#define __init __attribute__((section(".init")))

extern int __stack_start__, __stack_end__;
extern int __stack_process_start__, __stack_process_end__;
extern int __bss_start__, __bss_end__;
extern int __data_start__, __data_end__, __data_load_start__;
extern int __tbss_start__, __tbss_end__;
extern int __tdata_start__, __tdata_end__, __tdata_load_start__;

const void *__machine_process_start = &__stack_process_start__;

__init void Reset_Handler() 
{
#ifdef DEBUG
	// initialize process stack
	__mem_set(&__stack_process_start__, &__stack_process_end__, 0xcc);
#endif

	// switch to process stack
	__set_PSP((uint32_t)&__stack_process_end__);
	__set_CONTROL(2);

#ifdef DEBUG
	// initialize system stack
	__mem_set(&__stack_start__, &__stack_end__, 0xcc);
#endif

	// initialize bss, data sections
	__mem_set(&__bss_start__, &__bss_end__, 0);
	__mem_copy(&__data_start__, &__data_end__, &__data_load_start__);

	// initialize main bss, data tls sections
	__mem_set(&__tbss_start__, &__tbss_end__, 0);
	__mem_copy(&__tdata_start__, &__tdata_end__, &__tdata_load_start__);

	SystemInit();

	__kernel_start();

	while(1);
}

__init void Default_Handler()
{
	__machine_reset();
}

// weak IRQ handler prototypes
void __weak NMI_Handler();
void __weak HardFault_Handler();
void __weak MemManage_Handler();
void __weak BusFault_Handler();
void __weak UsageFault_Handler();
void __weak SVC_Handler();
void __weak DebugMon_Handler();
void __weak PendSV_Handler();
void __weak SysTick_Handler();
void __weak WWDG_IRQHandler();
void __weak PVD_IRQHandler();
void __weak TAMP_STAMP_IRQHandler();
void __weak RTC_WKUP_IRQHandler();
void __weak FLASH_IRQHandler();
void __weak RCC_IRQHandler();
void __weak EXTI0_IRQHandler();
void __weak EXTI1_IRQHandler();
void __weak EXTI2_IRQHandler();
void __weak EXTI3_IRQHandler();
void __weak EXTI4_IRQHandler();
void __weak DMA1_Stream0_IRQHandler();
void __weak DMA1_Stream1_IRQHandler();
void __weak DMA1_Stream2_IRQHandler();
void __weak DMA1_Stream3_IRQHandler();
void __weak DMA1_Stream4_IRQHandler();
void __weak DMA1_Stream5_IRQHandler();
void __weak DMA1_Stream6_IRQHandler();
void __weak ADC_IRQHandler();
void __weak CAN1_TX_IRQHandler();
void __weak CAN1_RX0_IRQHandler();
void __weak CAN1_RX1_IRQHandler();
void __weak CAN1_SCE_IRQHandler();
void __weak EXTI9_5_IRQHandler();
void __weak TIM1_BRK_TIM9_IRQHandler();
void __weak TIM1_UP_TIM10_IRQHandler();
void __weak TIM1_TRG_COM_TIM11_IRQHandler();
void __weak TIM1_CC_IRQHandler();
void __weak TIM2_IRQHandler();
void __weak TIM3_IRQHandler();
void __weak TIM4_IRQHandler();
void __weak I2C1_EV_IRQHandler();
void __weak I2C1_ER_IRQHandler();
void __weak I2C2_EV_IRQHandler();
void __weak I2C2_ER_IRQHandler();
void __weak SPI1_IRQHandler();
void __weak SPI2_IRQHandler();
void __weak USART1_IRQHandler();
void __weak USART2_IRQHandler();
void __weak USART3_IRQHandler();
void __weak EXTI15_10_IRQHandler();
void __weak RTC_Alarm_IRQHandler();

void __weak OTG_FS_WKUP_IRQHandler();
void __weak TIM8_BRK_TIM12_IRQHandler();
void __weak TIM8_UP_TIM13_IRQHandler();
void __weak TIM8_TRG_COM_TIM14_IRQHandler();
void __weak TIM8_CC_IRQHandler();
void __weak DMA1_Stream7_IRQHandler();
void __weak FMC_IRQHandler();
void __weak SDIO_IRQHandler();
void __weak TIM5_IRQHandler();
void __weak SPI3_IRQHandler();
void __weak UART4_IRQHandler();
void __weak UART5_IRQHandler();
void __weak TIM6_DAC_IRQHandler();
void __weak TIM7_IRQHandler();
void __weak DMA2_Stream0_IRQHandler();
void __weak DMA2_Stream1_IRQHandler();
void __weak DMA2_Stream2_IRQHandler();
void __weak DMA2_Stream3_IRQHandler();
void __weak DMA2_Stream4_IRQHandler();
void __weak ETH_IRQHandler();
void __weak ETH_WKUP_IRQHandler();
void __weak CAN2_TX_IRQHandler();
void __weak CAN2_RX0_IRQHandler();
void __weak CAN2_RX1_IRQHandler();
void __weak CAN2_SCE_IRQHandler();
void __weak OTG_FS_IRQHandler();
void __weak DMA2_Stream5_IRQHandler();
void __weak DMA2_Stream6_IRQHandler();
void __weak DMA2_Stream7_IRQHandler();
void __weak USART6_IRQHandler();
void __weak I2C3_EV_IRQHandler();
void __weak I2C3_ER_IRQHandler();
void __weak OTG_HS_EP1_OUT_IRQHandler();
void __weak OTG_HS_EP1_IN_IRQHandler();
void __weak OTG_HS_WKUP_IRQHandler();
void __weak OTG_HS_IRQHandler();
void __weak DCMI_IRQHandler();
void __weak CRYP_IRQHandler();
void __weak HASH_RNG_IRQHandler();
void __weak FPU_IRQHandler();
void __weak UART7_IRQHandler();
void __weak UART8_IRQHandler();
void __weak SPI4_IRQHandler();
void __weak SPI5_IRQHandler();
void __weak SPI6_IRQHandler();
void __weak SAI1_IRQHandler();
void __weak LTDC_IRQHandler();
void __weak LTDC_ER_IRQHandler();
void __weak DMA2D_IRQHandler();
void __weak QUADSPI_IRQHandler();
void __weak DSI_IRQHandler();

#pragma weak NMI_Handler = Default_Handler
#pragma weak HardFault_Handler = Default_Handler
#pragma weak MemManage_Handler = Default_Handler
#pragma weak BusFault_Handler = Default_Handler
#pragma weak UsageFault_Handler = Default_Handler
#pragma weak SVC_Handler = Default_Handler
#pragma weak DebugMon_Handler = Default_Handler
#pragma weak PendSV_Handler = Default_Handler
#pragma weak SysTick_Handler = Default_Handler

#pragma weak WWDG_IRQHandler = Default_Handler
#pragma weak PVD_IRQHandler = Default_Handler
#pragma weak TAMP_STAMP_IRQHandler = Default_Handler
#pragma weak RTC_WKUP_IRQHandler = Default_Handler
#pragma weak FLASH_IRQHandler = Default_Handler
#pragma weak RCC_IRQHandler = Default_Handler
#pragma weak EXTI0_IRQHandler = Default_Handler
#pragma weak EXTI1_IRQHandler = Default_Handler
#pragma weak EXTI2_IRQHandler = Default_Handler
#pragma weak EXTI3_IRQHandler = Default_Handler
#pragma weak EXTI4_IRQHandler = Default_Handler
#pragma weak DMA1_Stream0_IRQHandler = Default_Handler
#pragma weak DMA1_Stream1_IRQHandler = Default_Handler
#pragma weak DMA1_Stream2_IRQHandler = Default_Handler
#pragma weak DMA1_Stream3_IRQHandler = Default_Handler
#pragma weak DMA1_Stream4_IRQHandler = Default_Handler
#pragma weak DMA1_Stream5_IRQHandler = Default_Handler
#pragma weak DMA1_Stream6_IRQHandler = Default_Handler
#pragma weak ADC_IRQHandler = Default_Handler
#pragma weak CAN1_TX_IRQHandler = Default_Handler
#pragma weak CAN1_RX0_IRQHandler = Default_Handler
#pragma weak CAN1_RX1_IRQHandler = Default_Handler
#pragma weak CAN1_SCE_IRQHandler = Default_Handler
#pragma weak EXTI9_5_IRQHandler = Default_Handler
#pragma weak TIM1_BRK_TIM9_IRQHandler = Default_Handler
#pragma weak TIM1_UP_TIM10_IRQHandler = Default_Handler
#pragma weak TIM1_TRG_COM_TIM11_IRQHandler = Default_Handler
#pragma weak TIM1_CC_IRQHandler = Default_Handler
#pragma weak TIM2_IRQHandler = Default_Handler
#pragma weak TIM3_IRQHandler = Default_Handler
#pragma weak TIM4_IRQHandler = Default_Handler
#pragma weak I2C1_EV_IRQHandler = Default_Handler
#pragma weak I2C1_ER_IRQHandler = Default_Handler
#pragma weak I2C2_EV_IRQHandler = Default_Handler
#pragma weak I2C2_ER_IRQHandler = Default_Handler
#pragma weak SPI1_IRQHandler = Default_Handler
#pragma weak SPI2_IRQHandler = Default_Handler
#pragma weak USART1_IRQHandler = Default_Handler
#pragma weak USART2_IRQHandler = Default_Handler
#pragma weak USART3_IRQHandler = Default_Handler
#pragma weak EXTI15_10_IRQHandler = Default_Handler
#pragma weak RTC_Alarm_IRQHandler = Default_Handler
#pragma weak OTG_FS_WKUP_IRQHandler = Default_Handler
#pragma weak TIM8_BRK_TIM12_IRQHandler = Default_Handler
#pragma weak TIM8_UP_TIM13_IRQHandler = Default_Handler
#pragma weak TIM8_TRG_COM_TIM14_IRQHandler = Default_Handler
#pragma weak TIM8_CC_IRQHandler = Default_Handler
#pragma weak DMA1_Stream7_IRQHandler = Default_Handler
#pragma weak FMC_IRQHandler = Default_Handler
#pragma weak SDIO_IRQHandler = Default_Handler
#pragma weak TIM5_IRQHandler = Default_Handler
#pragma weak SPI3_IRQHandler = Default_Handler
#pragma weak UART4_IRQHandler = Default_Handler
#pragma weak UART5_IRQHandler = Default_Handler
#pragma weak TIM6_DAC_IRQHandler = Default_Handler
#pragma weak TIM7_IRQHandler = Default_Handler
#pragma weak DMA2_Stream0_IRQHandler = Default_Handler
#pragma weak DMA2_Stream1_IRQHandler = Default_Handler
#pragma weak DMA2_Stream2_IRQHandler = Default_Handler
#pragma weak DMA2_Stream3_IRQHandler = Default_Handler
#pragma weak DMA2_Stream4_IRQHandler = Default_Handler
#pragma weak ETH_IRQHandler = Default_Handler
#pragma weak ETH_WKUP_IRQHandler = Default_Handler
#pragma weak CAN2_TX_IRQHandler = Default_Handler
#pragma weak CAN2_RX0_IRQHandler = Default_Handler
#pragma weak CAN2_RX1_IRQHandler = Default_Handler
#pragma weak CAN2_SCE_IRQHandler = Default_Handler
#pragma weak OTG_FS_IRQHandler = Default_Handler
#pragma weak DMA2_Stream5_IRQHandler = Default_Handler
#pragma weak DMA2_Stream6_IRQHandler = Default_Handler
#pragma weak DMA2_Stream7_IRQHandler = Default_Handler
#pragma weak USART6_IRQHandler = Default_Handler
#pragma weak I2C3_EV_IRQHandler = Default_Handler
#pragma weak I2C3_ER_IRQHandler = Default_Handler
#pragma weak OTG_HS_EP1_OUT_IRQHandler = Default_Handler
#pragma weak OTG_HS_EP1_IN_IRQHandler = Default_Handler
#pragma weak OTG_HS_WKUP_IRQHandler = Default_Handler
#pragma weak OTG_HS_IRQHandler = Default_Handler
#pragma weak DCMI_IRQHandler = Default_Handler
#pragma weak CRYP_IRQHandler = Default_Handler
#pragma weak HASH_RNG_IRQHandler = Default_Handler
#pragma weak FPU_IRQHandler = Default_Handler
#pragma weak UART7_IRQHandler = Default_Handler
#pragma weak UART8_IRQHandler = Default_Handler
#pragma weak SPI4_IRQHandler = Default_Handler
#pragma weak SPI5_IRQHandler = Default_Handler
#pragma weak SPI6_IRQHandler = Default_Handler
#pragma weak SAI1_IRQHandler = Default_Handler
#pragma weak LTDC_IRQHandler = Default_Handler
#pragma weak LTDC_ER_IRQHandler = Default_Handler
#pragma weak DMA2D_IRQHandler = Default_Handler
#pragma weak QUADSPI_IRQHandler = Default_Handler
#pragma weak DSI_IRQHandler = Default_Handler
