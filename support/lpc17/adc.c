// LPC17xx ADC Peripheral Support
// by Miguel Fides

#include "adc.h"
#include "cpu.h"
#include <support/adc_hal.h>
#include <support/board_hal.h>

static ADC_MODULE *_adc = (ADC_MODULE *)LPC_ADC;
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

    LPC_SC->PCONP |= PCONP_PCADC;
	int pclk = cpu_pclk(SystemCoreClock, 1);

	_adc->CR = 0;	//reset module
	_adc->CR = (((pclk / clks) - 1) << ADCR_CLKDIV_BIT) 
#ifdef ADC_BURST_MODE
		| (_mask << ADCR_SEL_BIT) | ADCR_BURST 
#endif
		| ADCR_PDN;

	/* NVIC_EnableIRQ(ADC_IRQn); */
	_adc->INTEN = 0;	// disable general DONE int
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
	unsigned long acr = _adc->CR & (0xFF << ADCR_CLKDIV_BIT); 
	_adc->CR = acr | ((1 << channel) << ADCR_SEL_BIT) | 
		(1 << ADCR_START_BIT) | ADCR_PDN;
	unsigned long gdr;
	do 
	{ 
		gdr = _adc->GDR; 
	} while(!(gdr & ADDR_DONE));
#endif
	value = _adc->DR[channel] & 0xFFFF;

	return value;
}
