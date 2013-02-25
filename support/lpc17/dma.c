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
static void *_callback_states[DMA_CHANNEL_COUNT];
static unsigned char _allocated[DMA_CHANNEL_COUNT];

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

		_allocated[ch] = 0;
	}

	// setup interrupts
	NVIC_EnableIRQ(DMA_IRQn);
}

int dma_alloc_channels(int *array, int length)
{
	int done = 0;
	for(int ch = 0; ch < DMA_CHANNEL_COUNT; ch++)
	{
		if (cpu_trylock(&_allocated[ch], 1))
		{
			array[done++] = ch;
			if (done == length) return 1;
		}
	}
	while(done > 0)
	{
		int ch = array[done];
		cpu_unlock(&_allocated[ch]);
	}
	return 0;
}

void dma_free_channel(int ch)
{
	dma_channel_disable(ch);
	cpu_unlock(&_allocated[ch]);
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
				callback(ch, tc_done, _callback_states[ch]);
			}
			LPC_GPDMA->DMACIntTCClear = ch_mask;
			LPC_GPDMA->DMACIntErrClr = ch_mask;
		}
	}
}

void dma_transfer_setup(DMA_TRANSFER *tr, void *src_ptr, void *dst_ptr, int size, 
	const DMA_CONFIG *config, DMA_CALLBACK callback, DMA_TRANSFER *next)
{
	tr->SrcAddr = (unsigned long)src_ptr;
	tr->DstAddr = (unsigned long)dst_ptr;
	tr->LLI = (unsigned long)next;
	int con = (size & 0xFFF)	// TransferSize
		| (config->Src.Burst << 12)	// Source Burst Size
		| (config->Dst.Burst << 15)	// Data Burst Size
		| (config->Src.Width << 18)	// Source Size
		| (config->Dst.Width << 21)	// Dest Size
		| (config->Src.Increment << 26)	// Src inc flag
		| (config->Dst.Increment << 27)	// Dst inc flag
		| (callback ? 0x80000000 : 0);	// Interrupt
	tr->Control = con;
}

void dma_channel_enable(int ch, const DMA_TRANSFER *tr, 
	const DMA_CONFIG *config, DMA_CALLBACK callback, void *state)
{
	if (ch < DMA_CHANNEL_COUNT)
	{
		DMA_CHANNEL *dma_ch = _channels[ch];
		int mask = (1 << ch);
		LPC_GPDMA->DMACIntTCClear |= mask;
		LPC_GPDMA->DMACIntErrClr |= mask;
		_callbacks[ch] = callback;
		_callback_states[ch] = state;

		dma_ch->Transfer = *tr;
		dma_ch->Configuration = 1 // enable
			| (config->Peripheral << 1) | (config->Peripheral << 6) // src and dest periph (ignored when it's memory)
			| (config->Flow << 11)	// mode (flow_control)
			| (callback ? 1 << 14 : 0) // Int Error Enable
			| (callback ? 1 << 15 : 0); // Int Term Count Enable
	}
}

void dma_channel_enable_fast(int ch, void *src_ptr, void *dst_ptr, int size, 
	const DMA_CONFIG *config, DMA_CALLBACK callback, void *state)
{
	if (ch < DMA_CHANNEL_COUNT)
	{
		DMA_CHANNEL *dma_ch = _channels[ch];
		int mask = (1 << ch);
		LPC_GPDMA->DMACIntTCClear |= mask;
		LPC_GPDMA->DMACIntErrClr |= mask;
		_callbacks[ch] = callback;
		_callback_states[ch] = state;

		dma_transfer_setup(&dma_ch->Transfer, src_ptr, dst_ptr, size, 
			config, callback, (DMA_TRANSFER *)0);

		dma_ch->Configuration = 1 // enable
			| (config->Peripheral << 1) | (config->Peripheral << 6) // src and dest periph (ignored when it's memory)
			| (config->Flow << 11)	// mode (flow_control)
			| (callback ? 1 << 14 : 0) // Int Error Enable
			| (callback ? 1 << 15 : 0); // Int Term Count Enable
	}
} 

void dma_channel_disable(int ch)
{
	if (ch < DMA_CHANNEL_COUNT)
	{
		DMA_CHANNEL *dma_ch = _channels[ch];
		dma_ch->ConfigurationBits.Enable = 0;
	}
}




