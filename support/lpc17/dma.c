// LPC17 GPDMA Support
// by Miguel Fides

#include "dma.h"
#include <kernel/machine/hal.h>

static DMA_CHANNEL *_channels[DMA_CHANNEL_COUNT] = { 
	(DMA_CHANNEL *)LPC_GPDMACH0_BASE, (DMA_CHANNEL *)LPC_GPDMACH1_BASE, 
	(DMA_CHANNEL *)LPC_GPDMACH2_BASE, (DMA_CHANNEL *)LPC_GPDMACH3_BASE, 
	(DMA_CHANNEL *)LPC_GPDMACH4_BASE, (DMA_CHANNEL *)LPC_GPDMACH5_BASE, 
	(DMA_CHANNEL *)LPC_GPDMACH6_BASE, (DMA_CHANNEL *)LPC_GPDMACH7_BASE};
static DMA_CALLBACK _callbacks[DMA_CHANNEL_COUNT];
static void *_callback_states[DMA_CHANNEL_COUNT];
static unsigned char _allocated[DMA_CHANNEL_COUNT];
static DMA_MODULE *const _dma = (DMA_MODULE *)LPC_GPDMA;

void dma_initialize()
{
	// module initialization
	LPC_SC->PCONP |= PCONP_PCGPDMA;	// power enable module

	// clear all interrupt flags
	_dma->IntTCClear = DMA_CHANNEL_MASK;
	_dma->IntErrClr = DMA_CHANNEL_MASK;
	_dma->Config = DMACConfig_E; // enable module, little endian mode
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
		if (__machine_trylock(&_allocated[ch], 1))
		{
			array[done++] = ch;
			if (done == length) return 1;
		}
	}
	while(done > 0)
	{
		int ch = array[done];
		__machine_unlock(&_allocated[ch]);
	}
	return 0;
}

void dma_free_channel(int ch)
{
	dma_channel_disable(ch);
	__machine_unlock(&_allocated[ch]);
}

void GPDMA_IRQHandler()
{
	int int_mask = _dma->IntStat;
	for(int ch = 0; ch < DMA_CHANNEL_COUNT; ch++)
	{
		int ch_mask = (1 << ch);
		if (int_mask & ch_mask)
		{
			DMA_CALLBACK callback = _callbacks[ch];
			if (callback)
			{
				DMA_CHANNEL *mod = _channels[ch];
				int tc_done = _dma->IntTCStat & ch_mask ? 1 : 0;
				callback(ch, tc_done, _callback_states[ch]);
			}
			_dma->IntTCClear = ch_mask;
			_dma->IntErrClr = ch_mask;
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
		_dma->IntTCClear |= mask;
		_dma->IntErrClr |= mask;
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
		_dma->IntTCClear |= mask;
		_dma->IntErrClr |= mask;
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




