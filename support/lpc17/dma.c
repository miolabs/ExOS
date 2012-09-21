// LPC17 GPDMA Support
// by Miguel Fides

#include "dma.h"
#include <CMSIS/LPC17xx.h>

static DMA_CHANNEL *_channels[DMA_CHANNEL_COUNT] = { 
	(DMA_CHANNEL *)0x50004100, (DMA_CHANNEL *)0x50004120, 
	(DMA_CHANNEL *)0x50004140, (DMA_CHANNEL *)0x50004160, 
	(DMA_CHANNEL *)0x50004180, (DMA_CHANNEL *)0x500041A0, 
	(DMA_CHANNEL *)0x500041C0, (DMA_CHANNEL *)0x500041E0};
static DMA_CALLBACK _callbacks[DMA_CHANNEL_COUNT];

void dma_initialize()
{
	// module initialization
	LPC_SC->PCONP |= PCONP_PCGPDMA;	// power enable module

	// clear all interrupt flags
	LPC_GPDMA->DMACIntTCClear = DMA_CHANNEL_MASK;
	LPC_GPDMA->DMACIntErrClr = DMA_CHANNEL_MASK;
	LPC_GPDMA->DMACConfig = DMACConfig_E; // enable module, little endian mode
	for (int ch = 0; ch < DMA_CHANNEL_COUNT; ch++) 
	{
		_callbacks[ch] = (void *)0;
		DMA_CHANNEL *mod = _channels[ch];
		mod->ConfigurationBits.Enable = 0;
	}

	// setup interrupts
	//isr_setup(INT_GPDMA, 7, _isr);
	NVIC_EnableIRQ(DMA_IRQn);
}

void GPDMA_IRQHandler()
{
	int int_mask = LPC_GPDMA->DMACIntStat;
	for(int ch = 0; ch < DMA_CHANNEL_COUNT; ch++)
	{
		int ch_mask = (1 << ch);
		if (int_mask & ch_mask)
		{
			DMA_CALLBACK callback = _callbacks[ch];
			if (callback)
			{
				DMA_CHANNEL *mod = _channels[ch];
				int tc_done = LPC_GPDMA->DMACIntTCStat & ch_mask ? 1 : 0;
				callback(ch, tc_done);
			}
			LPC_GPDMA->DMACIntTCClear = ch_mask;
			LPC_GPDMA->DMACIntErrClr = ch_mask;
		}
	}
}

// enable a dma channel to be used in a hw2m or m2hw transaction
DMA_CHANNEL *dma_init_channel(int ch, int mode, void *src_ptr, void *dst_ptr, int size, 
	DMA_CON src, DMA_CON dst, int peripheral, DMA_CALLBACK callback)
{
	DMA_CHANNEL *mod = ch < DMA_CHANNEL_COUNT ? _channels[ch] : (DMA_CHANNEL *)0;
	if (mod)
	{
		int mask = (1 << ch);
		LPC_GPDMA->DMACIntTCClear |= mask;
		LPC_GPDMA->DMACIntErrClr |= mask;
		_callbacks[ch] = callback;

		mod->SrcAddr = (unsigned long)src_ptr;
		mod->DstAddr = (unsigned long)dst_ptr;
		int con = (size & 0xFFF)	// TransferSize
			| (src.Burst << 12)	// Source Burst Size
			| (dst.Burst << 15)	// Data Burst Size
			| (src.Width << 18)	// Source Size
			| (dst.Width << 21)	// Dest Size
			| (src.Increment << 26)	// Src inc flag
			| (dst.Increment << 27)	// Dst inc flag
			| (callback ? 0x80000000 : 0);	// Interrupt
		mod->Control = con;
	
		mod->Configuration = 1 // enable
			| (peripheral << 1) | (peripheral << 6) // src and dest periph (ignored when it's memory)
			| (mode << 11)	// mode (flow_control)
			| (callback ? 1 << 14 : 0) // Int Error Enable
			| (callback ? 1 << 15 : 0); // Int Term Count Enable
	}
	return mod;
} 

void dma_disable(DMA_CHANNEL *mod)
{
	mod->ConfigurationBits.Enable = 0;
}




