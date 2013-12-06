#include "cpu.h"
#include "ssp.h"
#include <support/ssp_hal.h>

static SSP_MODULE *_modules[] = { (SSP_MODULE *)LPC_SSP0_BASE, (SSP_MODULE *)LPC_SSP1_BASE };

void hal_ssp_initialize(int module, int bitrate, HAL_SSP_MODE mode, HAL_SSP_FLAGS flags)
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
			return;
	}

	SSP_MODULE *ssp = _modules[module];

	int prescale = SystemCoreClock / bitrate;
	int prescale_big = ((prescale / 128) + 1) << 1;
	int prescale_small = prescale / prescale_big;

	ssp->CR1 = 0;	// disable

	int bit_width = flags & HAL_SSP_LENGTH_16BIT ? 16 : 8;
	SSP_CLK_MODE ssp_mode = flags & HAL_SSP_CLK_IDLE_HIGH ? 
		(flags & HAL_SSP_CLK_PHASE_NEG ? SSP_CLK_POL1_PHA0 : SSP_CLK_POL1_PHA1) : // clk negative
		(flags & HAL_SSP_CLK_PHASE_NEG ? SSP_CLK_POL0_PHA1 : SSP_CLK_POL0_PHA0); // clk positive
	ssp->CR0 = (ssp_mode << 6) |
		((bit_width - 1) & 0xf) |	// DSS
		((prescale_small - 1) << 8);	// SCR
	ssp->CPSR = prescale_big;

	ssp->CR1 = SSP_CR1_SSE;
}

void hal_ssp_transmit(int module, unsigned char *outbuf, unsigned char *inbuf, int length)
{
	SSP_MODULE *ssp = _modules[module];

	//while(ssp->SRbits.RNE) { char dummy = ssp->DR; }
	int input_index = 0, output_index = 0;
    int output_offset = 0;
	while(input_index < length)
	{
		if (output_offset < 8 &&
			output_index < length &&
			ssp->SRbits.TNF)
		{
			ssp->DR = outbuf[output_index++];
			output_offset++;
		}
		if (ssp->SRbits.RNE) 
		{
			inbuf[input_index++] = ssp->DR;
			output_offset--;
		}
	}
}

void hal_ssp_sel_control(int module, int asserted)
{
}





