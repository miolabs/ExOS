// Cortex M0 adc support

#include "adc.h"
#include "cpu.h"
#include <support/adc_hal.h>
#include <support/board_hal.h>

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

	// Power
    LPC_SYSCON->PDRUNCFG = LPC_SYSCON->PDRUNCFG & 0xffffffef;  // Enable ADC block

	// AD conf.
	// Accurate conversion requires 11 cycles
	int clks = rate * 11; // * ch_count;
	int pclk = cpu_pclk(SystemCoreClock, 0);
	int clkdiv = (pclk / clks) - 1;
	assert( clkdiv < 0x100);
	LPC_ADC->CR = _mask | // AD pins to be sampled
				  (clkdiv<<8) | //clkdiv
				  (0<<16) | // burst mode
				  (0<<17) | // Clocks (0=11 clocks, max. precision)
				  (0<<24) | // burst start, no
				  (0<<27);  // burst edge start

	LPC_ADC->INTEN = 0;	// No interrupts

	// Enable clock
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<13);	// Enable ADC clock
}

void ADC_IRQHandler()
{
}

unsigned short hal_adc_read(int index)
{
	int channel = _ch_table[index & 0x7];
	// = LPC_ADC->GDR; Global data register

	unsigned st = LPC_ADC->STAT;
	while (( st & (1<<index)) != 0);

	unsigned short v = ((LPC_ADC->DR[channel] >> 6) & 0x3ff);
	return v; // 10 bits AD value
}
	/*unsigned short value;
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
*/
