// LPC17xx SSP Peripheral Support
// by Miguel Fides

#include "ssp.h"
#include "cpu.h"
#include "dma.h"
#include <support/ssp_hal.h>
#include <support/board_hal.h>
#include <CMSIS/LPC17xx.h>

static unsigned char _dma_busy[SSP_MODULE_COUNT];

static inline SSP_MODULE *_get_module(int module)
{
	switch(module)
	{
		case 0: return (SSP_MODULE *)LPC_SSP0;
		case 1: return (SSP_MODULE *)LPC_SSP1;
	}
	return (SSP_MODULE *)0;
}

void hal_ssp_initialize(int module, int bitrate, HAL_SSP_MODE mode, HAL_SSP_FLAGS flags)
{
	SSP_MODULE *ssp = _get_module(module);
	if (ssp && hal_board_init_pinmux(HAL_RESOURCE_SSP, module))
	{
		int pclk_div;
		switch(module)
		{
			case 0:
				pclk_div = PCLKSEL1bits.PCLK_SSP0;
				break;
			case 1:
				pclk_div = PCLKSEL0bits.PCLK_SSP1;
				break;
		}
		SSP_CLK_MODE mode = flags & HAL_SSP_CLK_IDLE_HIGH ? 
			(flags & HAL_SSP_CLK_PHASE_NEG ? SSP_CLK_POL1_PHA0 : SSP_CLK_POL1_PHA1) : // clk negative
			(flags & HAL_SSP_CLK_PHASE_NEG ? SSP_CLK_POL0_PHA1 : SSP_CLK_POL0_PHA0); // clk positive

		ssp->CR1 = 0;	// disable module
		ssp->CPSR = 2;	// prescaler
	
		ssp->CR0 = (SSP_DSS_8BIT << SSPCR0_DSS_BIT) |
			(SSP_FRF_SPI << SSPCR0_DSS_BIT) |
			(mode << SSPCR0_CLKMODE_BIT);
		ssp->CR1 = SSPCR1_SSE;	// master mode, enable module
	
		unsigned long pclk = cpu_pclk(SystemCoreClock, pclk_div);	// required PCLK = CCLK
		unsigned long scr = (pclk / (ssp->CPSR * bitrate)) - 1;
		ssp->CR0 = (ssp->CR0 & ~(0xFF << SSPCR0_SCR_BIT)) | 
			((scr & 0xFF) << SSPCR0_SCR_BIT);

	}
}

static inline void _dma_complete(int ch, int tc_done, void *state)
{
	int module = (int)state;
	_dma_busy[module] = 0;
	ssp_dma_set_event(module);
}

static inline void _dma_begin(int module)
{
	_dma_busy[module] = 1;
	ssp_dma_reset_event(module);
}

static inline void _dma_wait(int module)
{
	while(_dma_busy[module])
		ssp_dma_wait(module);
}

static const unsigned char _rx_periph[] = { DMA_P_SSP0_RX, DMA_P_SSP1_RX };
static const unsigned char _tx_periph[] = { DMA_P_SSP0_TX, DMA_P_SSP1_TX };

void hal_ssp_transmit(int module, unsigned char *outbuf, unsigned char *inbuf, int length)
{
	static unsigned long dummy;
	SSP_MODULE *ssp = _get_module(module);
	if (ssp)
	{
		int dma[2];
		if (dma_alloc_channels(dma, 2))
		{
			_dma_begin(module);
			
			DMA_CONFIG rx_config;
			if (inbuf)
			{
				rx_config = (DMA_CONFIG) {
					.Src = { .Burst = DMA_BURST_4, .Width = DMA_WIDTH_8BIT },
					.Dst = { .Burst = DMA_BURST_4, .Width = DMA_WIDTH_8BIT, .Increment = 1 },
					.Peripheral = _rx_periph[module], 
					.Flow = DMA_FLOW_P2M_DMA };
				dma_channel_enable_fast(dma[0], (void *)&ssp->DR, inbuf, length, 
					&rx_config, _dma_complete, (void *)module);
			}
			else
			{
				rx_config = (DMA_CONFIG) {
					.Src = { .Burst = DMA_BURST_4, .Width = DMA_WIDTH_8BIT },
					.Dst = { .Burst = DMA_BURST_4, .Width = DMA_WIDTH_8BIT },
					.Peripheral = _rx_periph[module], 
					.Flow = DMA_FLOW_P2M_DMA };
				dma_channel_enable_fast(dma[0], (void *)&ssp->DR, &dummy, length, 
					&rx_config, _dma_complete, (void *)module);
			}

            DMA_CONFIG tx_config;
			if (outbuf)
			{
				tx_config = (DMA_CONFIG) {
					.Src = { .Burst = DMA_BURST_4, .Width = DMA_WIDTH_8BIT, .Increment = 1 },
					.Dst = { .Burst = DMA_BURST_4, .Width = DMA_WIDTH_8BIT },
					.Peripheral = _tx_periph[module], 
					.Flow = DMA_FLOW_M2P_DMA };
				dma_channel_enable_fast(dma[1], outbuf, (void *)&ssp->DR, length, 
					&tx_config, 0, 0);
			}
			else
			{
				tx_config = (DMA_CONFIG) {
					.Src = { .Burst = DMA_BURST_4, .Width = DMA_WIDTH_8BIT },
					.Dst = { .Burst = DMA_BURST_4, .Width = DMA_WIDTH_8BIT },
					.Peripheral = _tx_periph[module], 
					.Flow = DMA_FLOW_M2P_DMA };
				dummy = 0;
				dma_channel_enable_fast(dma[1], &dummy, (void *)&ssp->DR, length, 
					&tx_config, 0, 0);
			}

			ssp->DMACR |= SSP_DMA_RXDMAE | SSP_DMA_TXDMAE;
			_dma_wait(module);
			ssp->DMACR = 0;

			dma_free_channel(dma[1]);
			dma_free_channel(dma[0]);
		}
	}
}

//void hal_ssp_transmit(int module, unsigned char *outbuf, unsigned char *inbuf, int length)
//{
//	SSP_MODULE *ssp = _get_module(module);
//	if (ssp)
//	{
//		while(ssp->SRbits.RNE) { char dummy = ssp->DR; }
//		int input_index = 0, output_index = 0;
//		int output_offset = 0;
//		while(input_index < length)
//		{
//			if (output_offset < 8 &&
//				output_index < length &&
//				ssp->SRbits.TNF)
//			{
//				ssp->DR = outbuf[output_index++];
//				output_offset++;
//			}
//			if (ssp->SRbits.RNE) 
//			{
//				inbuf[input_index++] = ssp->DR;
//				output_offset--;
//			}
//		}
//	}
//}




