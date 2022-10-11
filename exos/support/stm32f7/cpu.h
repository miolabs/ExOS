#ifndef STM32F7_CPU_H
#define STM32F7_CPU_H

#include <stm32f7xx.h>

#define RCC_CFGR_SW_BIT		0
#define RCC_CFGR_SWS_BIT	2
#define RCC_CFGR_HPRE_BIT	4
#define RCC_CFGR_PPRE1_BIT	10
#define RCC_CFGR_PPRE2_BIT	13
#define RCC_CFGR_RTCPRE_BIT	16
#define RCC_CFGR_MCO1_BIT	21
#define RCC_CFGR_I2SSRC_BIT	23
#define RCC_CFGR_MCO1PRE_BIT	24
#define RCC_CFGR_MCO2PRE_BIT	27
#define RCC_CFGR_MCO2_BIT	30

#define RCC_PLLCFGR_PLLM_BIT	0
#define RCC_PLLCFGR_PLLN_BIT	6
#define RCC_PLLCFGR_PLLP_BIT	16
#define RCC_PLLCFGR_PLLSRC_BIT	22
#define RCC_PLLCFGR_PLLQ_BIT	24
#if defined STM32F769xx || defined STM32F779xx
#define RCC_PLLCFGR_PLLR_BIT	28	
#endif

#if defined STM32F769xx || defined STM32F779xx
#define RCC_DCKCFGR1_PLLSAIDIVR_BIT 16
#endif

#define RCC_PLLI2SCFGR_PLLN_BIT 6
#define RCC_PLLI2SCFGR_PLLQ_BIT 24
#define RCC_PLLI2SCFGR_PLLR_BIT 28

typedef enum
{
	MCO1_SOURCE_SYSCLK = 0,
	MCO1_SOURCE_PLLI2S,
	MCO1_SOURCE_HSE,
	MCO1_SOURCE_PLL,
} mco1_source_t;

typedef enum
{
	MCO2_SOURCE_HSI = 0,
	MCO2_SOURCE_LSE,
	MCO2_SOURCE_HSE,
	MCO2_SOURCE_PLL,
} mco2_source_t;

unsigned cpu_get_pclk1();
unsigned cpu_get_pclk2();
unsigned cpu_get_pll_input();
unsigned cpu_get_pll_qclk();
unsigned cpu_get_sdioclk();

unsigned cpu_setup_plli2s(unsigned n, unsigned q, unsigned r); // NOTE: vco = input * n/m(from main pll); i2s = vco / r; sai1 = vco / q;
void cpu_setup_mco2(mco2_source_t src, unsigned div);

#endif // STM32F7_CPU_H



