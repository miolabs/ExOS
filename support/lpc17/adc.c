// LPC17xx ADC Peripheral Support
// by Miguel Fides

#include "adc.h"
#include "cpu.h"
#include <support/adc_hal.h>
#include <support/board_hal.h>
#include <kernel/thread.h>
#include <CMSIS/LPC17xx.h>

static unsigned char _mask;
static unsigned char _ch_table[8];

unsigned long hal_adc_initialize(int rate, int bits)
{	
	_mask = (unsigned char)hal_board_init_pinmux(HAL_RESOURCE_ADC, 0);
	int ch = 0;
	for (int bit = 0; bit < 8; bit++)
	{
		if (_mask & (1 << bit)) _ch_table[ch++] = bit;
	}
	int ch_count = ch;
	while(ch < 8)
	{
		_ch_table[ch++] = 0;
	}

	int clks = rate * ch_count * 65; 

	int pclk;
	PCLKSEL0bits.PCLK_ADC = 0; // PCLK = CCLK / 4
    LPC_SC->PCONP |= PCONP_PCADC;
	pclk = cpu_pclk(SystemCoreClock, 0);

	LPC_ADC->ADCR = 0;	//reset module
	LPC_ADC->ADCR = (((pclk / clks) - 1) << ADCR_CLKDIV_BIT) 
#ifdef ADC_BURST_MODE
		| (_mask << ADCR_SEL_BIT) | ADCR_BURST 
#endif
		| ADCR_PDN;

	//NVIC_EnableIRQ(ADC_IRQn);
	LPC_ADC->ADINTEN = 0;	// disable general DONE int
	return _mask;
}

void ADC_IRQHandler()
{
}

unsigned short hal_adc_read(int index)
{
	unsigned short value;
	int channel = _ch_table[index & 0x7];

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
