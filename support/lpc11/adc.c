// Cortex M0 adc support

#include "adc.h"
#include "cpu.h"
#include <support/adc_hal.h>
#include <support/board_hal.h>
#include <assert.h>

static unsigned char _ch_table[8];

unsigned long hal_adc_initialize(int rate, int bits)
{	
	LPC_SYSCON->PDRUNCFG &= ~PDRUNCFG_ADC_PD;  // Enable ADC block
	LPC_SYSCON->SYSAHBCLKCTRL |= SYSAHBCLKCTRL_ADC;	// Enable ADC clock

	// AD conf.
	// Accurate conversion requires 11 cycles
	int freq = rate * 11;
	if (freq > 4500000) freq = 4500000;
	int clkdiv = ((SystemCoreClock + (freq - 1)) / freq) - 1;
	if (clkdiv > 0xFF) clkdiv = 0xFF;
	
#ifdef ADC_BURST_MASK
	LPC_ADC->CR = ADC_BURST_MASK | // AD pins to be sampled
					(clkdiv << ADCR_CLKDIV_BIT) | //clkdiv
					ADCR_BURST | // burst mode
					(0 << ADCR_CLKS_BIT) | // Clocks (0=11 clocks, max. precision)
					(0 << ADCR_START_BIT); // sw start
#else
	LPC_ADC->CR = (clkdiv << ADCR_CLKDIV_BIT) | //clkdiv
					(0 << ADCR_CLKS_BIT) | // Clocks (0=11 clocks, max. precision)
					(0 << ADCR_START_BIT); // sw start
#endif	
	LPC_ADC->INTEN = 0;	// No interrupts
	return 1;
}

void ADC_IRQHandler()
{
}

static int _read(int channel, unsigned short *presult)
{
	unsigned long dr = LPC_ADC->DR[channel];
	*presult = dr & 0xFFC0;
	return dr & (1<<31);
}

unsigned short hal_adc_read(int channel)
{
	channel &= 0x7;
#ifndef ADC_BURST_MASK
	LPC_ADC->CR = (LPC_ADC->CR & ADCR_CLKDIV_MASK) | (1 << channel) | (ADCR_START_NOW << ADCR_START_BIT);
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
	LPC_ADC->CR = (LPC_ADC->CR & ADCR_CLKDIV_MASK) | (1 << channel) | (start << ADCR_START_BIT);
	return channel;
}

