#ifndef LPC11_ADC_H
#define LPC11_ADC_H

#define ADCR_SEL_BIT 0
#define ADCR_SEL_MASK 0x000000FF
#define ADCR_CLKDIV_BIT 8
#define ADCR_CLKDIV_MASK 0x0000FF00
#define ADCR_BURST (1<<16)
#define ADCR_START_BIT 24

#define ADDR_DONE (1<<31)
#define ADDR_OVERRUN (1<<30)
#define ADDR_CHN_MASK 0x07000000
#define ADDR_CHN_BIT 24


#endif //LPC11_ADC_H

