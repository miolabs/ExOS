#ifndef LPC2K_ADC_H
#define LPC2K_ADC_H

#include <support/lpc2k/cpu.h>

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

typedef struct
{
  __IO unsigned long ADCR;
  __IO unsigned long ADGDR;
       unsigned long RESERVED0;
  __IO unsigned long ADINTEN;
  __I  unsigned long ADDR0;
  __I  unsigned long ADDR1;
  __I  unsigned long ADDR2;
  __I  unsigned long ADDR3;
  __I  unsigned long ADDR4;
  __I  unsigned long ADDR5;
  __I  unsigned long ADDR6;
  __I  unsigned long ADDR7;
  __I  unsigned long ADSTAT;
  __IO unsigned long ADTRM;
} LPC_ADC_TypeDef;

extern LPC_ADC_TypeDef *LPC_ADC;

#endif //LPC2K_ADC_H

