#include "cpu.h"
#include "ssp.h"

static SSP_MODULE *_modules[] = { (SSP_MODULE *)LPC_SSP0_BASE, (SSP_MODULE *)LPC_SSP1_BASE };

SSP_MODULE *ssp_initialize(int module, int freq, int bit_width)
{
	switch(module)
	{
		case 0:
			LPC_SYSCON->SSP0CLKDIV = 1;
			LPC_SYSCON->SYSAHBCLKCTRL |= SYSAHBCLKCTRL_SSP0;
			LPC_SYSCON->PRESETCTRL |= PRESETCTRL_SSP0_RST_N;
			break;
		case 1:
			LPC_SYSCON->SSP1CLKDIV = 1;
			LPC_SYSCON->SYSAHBCLKCTRL |= SYSAHBCLKCTRL_SSP1;
			LPC_SYSCON->PRESETCTRL |= PRESETCTRL_SSP1_RST_N;
			break;
		default:
			return 0;
	}

	SSP_MODULE *ssp = _modules[module];

	int prescale = SystemCoreClock / freq;
	int prescale_big = ((prescale / 128) + 1) << 1;
	int prescale_small = prescale / prescale_big;

	ssp->CR1.Value = 0;	// disable

	ssp->CR0.Value = SSP_CR0_CPHA | SSP_CR0_CPOL |
		((bit_width - 1) & 0xf) |	// DSS
		((prescale_small - 1) << 8);	// SCR
	ssp->CPSR = prescale_big;

	ssp->CR1.Value = SSP_CR1_SSE;
	return ssp;
}




