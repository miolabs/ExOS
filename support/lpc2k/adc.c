// LPC23xx/24xx ADC Peripheral Support
// by Miguel Fides

#include "adc.h"
#include "cpu.h"
#include <support/adc_hal.h>
#include <support/board_hal.h>

LPC_ADC_TypeDef *LPC_ADC = (LPC_ADC_TypeDef *)0xE0034000;

unsigned long hal_adc_initialize(int rate, int bits)
{	
	int pclk;
	PCLKSEL0bits.PCLK_ADC = 0; // PCLK = CCLK / 4
    LPC_SC->PCONP |= PCONP_PCADC;
	pclk = cpu_pclk(SystemCoreClock, 0);

	int freq = rate * 65;
	if (freq > ADC_MAX_CLK) freq = ADC_MAX_CLK;
	int clkdiv = ((SystemCoreClock + (freq - 1)) / freq) - 1;
	if (clkdiv > 0xFF) clkdiv = 0xFF;

	LPC_ADC->ADCR = 0;	//reset module
	LPC_ADC->ADCR = (clkdiv << ADCR_CLKDIV_BIT) 
#ifdef ADC_BURST_MODE
		| (ADC_BURST_MASK << ADCR_SEL_BIT) | ADCR_BURST 
#endif
		| ADCR_PDN;

	//NVIC_EnableIRQ(ADC_IRQn);
	LPC_ADC->ADINTEN = 0;	// disable general DONE int
	return 1;
}

void ADC_IRQHandler()
{
}

unsigned short hal_adc_read(int channel)
{
	unsigned short value;

#ifndef ADC_BURST_MODE
	unsigned long acr = LPC_ADC->ADCR & (0xFF << ADCR_CLKDIV_BIT); 
	LPC_ADC->ADCR = acr | ((1 << channel) << ADCR_SEL_BIT) | 
		(1 << ADCR_START_BIT) | ADCR_PDN;
	unsigned long gdr;
	do 
	{ 
		gdr = LPC_ADC->ADGDR; 
	} while(!(gdr & ADDR_DONE));
#endif
	value = ((unsigned long *)&LPC_ADC->ADDR0)[channel] & 0xFFFF;

	return value;
}
