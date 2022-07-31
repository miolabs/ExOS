#ifndef LPC17_ADC_H
#define LPC17_ADC_H

#include <support/adc_hal.h>

#define ADC_MAX_CLK 13000000

typedef struct
{
	volatile unsigned int CR;                     /*!< Offset: 0x000       A/D Control Register (R/W) */
	volatile unsigned int GDR;                    /*!< Offset: 0x004       A/D Global Data Register (R/W) */
	volatile unsigned int RESERVED0;
	volatile unsigned int INTEN;                  /*!< Offset: 0x00C       A/D Interrupt Enable Register (R/W) */
	volatile unsigned int DR[8];                  /*!< Offset: 0x010-0x02C A/D Channel 0..7 Data Register (R/W) */
	volatile unsigned int STAT;                   /*!< Offset: 0x030       A/D Status Register (R/ ) */
	volatile unsigned int ADTRM;
} ADC_MODULE;

#define ADCR_SEL_BIT 0
#define ADCR_SEL_MASK (0xFF << ADCR_SEL_BIT)
#define ADCR_CLKDIV_BIT 8
#define ADCR_CLKDIV_MASK (0xFF << ADCR_CLKDIV_BIT)
#define ADCR_BURST (1<<16)
#define ADCR_CLKS_BIT 17
#define ADCR_PDN (1<<21)
#define ADCR_START_BIT 24

#define ADCR_START_NOW 1
#define ADCR_START_EINT0 2
#define ADCR_START_CAP0_1 3
#define ADCR_START_MAT0_1 4
#define ADCR_START_MAT0_3 5
#define ADCR_START_MAT1_0 6
#define ADCR_START_MAT1_1 7
#define ADCR_START_FALLING_EDGE 8

#define ADDR_DONE (1<<31)
#define ADDR_OVERRUN (1<<30)
#define ADDR_CHN_MASK 0x07000000
#define ADDR_CHN_BIT 24

int adc_start_conversion(int index, int start);

#endif //LPC17_ADC_H

