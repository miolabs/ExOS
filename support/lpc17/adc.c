// LPC17xx ADC Peripheral Support
// by Miguel Fides

#include "adc.h"
#include "cpu.h"
#include <support/adc_hal.h>
#include <support/board_hal.h>

static ADC_MODULE *_adc = (ADC_MODULE *)LPC_ADC;

unsigned long hal_adc_initialize(int rate, int bits)
{	
    LPC_SC->PCONP |= PCONP_PCADC;
	int pclk = cpu_pclk(SystemCoreClock, 0);

	// AD conf.
	// Accurate conversion requires 65 cycles
	int freq = rate * 65;
	if (freq > ADC_MAX_CLK) freq = ADC_MAX_CLK;
	int clkdiv = ((SystemCoreClock + (freq - 1)) / freq) - 1;
	if (clkdiv > 0xFF) clkdiv = 0xFF;

#ifdef ADC_BURST_MASK
	_adc->CR = ADC_BURST_MASK | // AD pins to be sampled
		(clkdiv << ADCR_CLKDIV_BIT) | //clkdiv
		ADCR_BURST | ADCR_PDN;
#else
	_adc->CR = (clkdiv << ADCR_CLKDIV_BIT) | //clkdiv
		ADCR_PDN;
#endif	
	_adc->INTEN = 0;	// No interrupts
	return 1;
}

void ADC_IRQHandler()
{
}

static int _read(int channel, unsigned short *presult)
{
	unsigned long dr = _adc->DR[channel];
	*presult = dr & 0xFFC0;
	return dr & (1<<31);
}

unsigned short hal_adc_read(int channel)
{
	channel &= 0x7;
#ifndef ADC_BURST_MASK
	_adc->CR = (_adc->CR & ADCR_CLKDIV_MASK) | (1 << channel) | ADCR_PDN | (ADCR_START_NOW << ADCR_START_BIT);
#endif
	unsigned short v;
	while(!_read(channel, &v));
	return v;
}

int hal_adc_read_no_wait(int channel, unsigned short *presult)
{
	return _read(channel & 0x7, presult);
}

int adc_start_conversion(int channel, int start)
{
	channel &= 0x7;
	_adc->CR = (_adc->CR & ADCR_CLKDIV_MASK) | (1 << channel) | ADCR_PDN | (start << ADCR_START_BIT);
	return channel;
}
