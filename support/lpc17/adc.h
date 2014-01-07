#ifndef LPC17_ADC_H
#define LPC17_ADC_H

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
#define ADCR_SEL_MASK 0x000000FF
#define ADCR_CLKDIV_BIT 8
#define ADCR_CLKDIV_MASK 0x0000FF00
#define ADCR_BURST (1<<16)
#define ADCR_PDN (1<<21)
#define ADCR_START_BIT 24

#define ADDR_DONE (1<<31)
#define ADDR_OVERRUN (1<<30)
#define ADDR_CHN_MASK 0x07000000
#define ADDR_CHN_BIT 24


#endif //LPC17_ADC_H

