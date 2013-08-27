// Cortex M0 adc support

#include "adc.h"
#include "cpu.h"
#include <support/adc_hal.h>
#include <support/board_hal.h>
#include <assert.h>

static unsigned char _mask;
static unsigned char _ch_table[8];

unsigned long hal_adc_initialize(int rate, int bits)
{	
	_mask = (unsigned char)hal_board_init_pinmux(HAL_RESOURCE_ADC, 0);
	_mask &= 0xff;
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

	LPC_SYSCON->PDRUNCFG &= ~PDRUNCFG_ADC_PD;  // Enable ADC block
	LPC_SYSCON->SYSAHBCLKCTRL |= SYSAHBCLKCTRL_ADC;	// Enable ADC clock

	if (ch_count != 0)
	{
		// AD conf.
		// Accurate conversion requires 11 cycles
		int clks = rate * 11 * ch_count;
		int clkdiv = (SystemCoreClock / clks) - 1;
		if (clkdiv > 0xFF) clkdiv = 0xFF;
	
#ifdef ADC_BURST_MODE
		LPC_ADC->CR = _mask | // AD pins to be sampled
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
	}
	return ch_count;
}

void ADC_IRQHandler()
{
}

unsigned short hal_adc_read(int index)
{
	int channel = _ch_table[index & 0x7];
#ifndef ADC_BURST_MODE
	LPC_ADC->CR = (LPC_ADC->CR & ADCR_CLKDIV_MASK) | (1 << channel) | (1 << ADCR_START_BIT);
#endif
	unsigned long dr;
	do { dr = LPC_ADC->DR[channel]; }
	while (!(dr & (1<<31)));
	unsigned short v = (dr & 0xFFC0); 
	return v; // 10 bits AD value
}
	