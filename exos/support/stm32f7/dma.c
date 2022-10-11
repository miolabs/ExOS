#include "dma.h"
#include "cpu.h"
#include <kernel/machine/hal.h>
#include <kernel/panic.h>

static DMA_TypeDef *const _modules[] = { DMA1, DMA2 };
static DMA_Stream_TypeDef *const _streams[2][8] = {
	{ DMA1_Stream0, DMA1_Stream1, DMA1_Stream2, DMA1_Stream3, DMA1_Stream4, DMA1_Stream5, DMA1_Stream6, DMA1_Stream7 },
	{ DMA2_Stream0, DMA2_Stream1, DMA2_Stream2, DMA2_Stream3, DMA2_Stream4, DMA2_Stream5, DMA2_Stream6, DMA2_Stream7 } };
static const char _irqn[2][8] = {
	{ DMA1_Stream0_IRQn, DMA1_Stream1_IRQn, DMA1_Stream2_IRQn, DMA1_Stream3_IRQn, DMA1_Stream4_IRQn, DMA1_Stream5_IRQn, DMA1_Stream6_IRQn, DMA1_Stream7_IRQn }, 
	{ DMA2_Stream0_IRQn, DMA2_Stream1_IRQn, DMA2_Stream2_IRQn, DMA2_Stream3_IRQn, DMA2_Stream4_IRQn, DMA2_Stream5_IRQn, DMA2_Stream6_IRQn, DMA2_Stream7_IRQn } }; 
static unsigned char _allocated[2][8];
static event_t *_events[2][8];

static void _initialize(unsigned module)
{
	switch(module)
	{
		case 0:
			RCC->AHB1RSTR |= RCC_AHB1RSTR_DMA1RST;
			RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
			RCC->AHB1RSTR ^= RCC_AHB1RSTR_DMA1RST;
			break;
		case 1:
			RCC->AHB1RSTR |= RCC_AHB1RSTR_DMA2RST;
			RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
			RCC->AHB1RSTR ^= RCC_AHB1RSTR_DMA2RST;
			break;
		default:
			kernel_panic(KERNEL_ERROR_KERNEL_PANIC);
	}

	for(unsigned si = 0; si < 8; si++)
	{
		_events[module][si] = nullptr;
		_allocated[module][si] = 0;

		DMA_Stream_TypeDef *s = _streams[module][si];
		s->CR = 0;
		s->FCR = 0;
	}
}

void dma_initialize()
{
	_initialize(0);	// DMA1	
	_initialize(1);	// DMA2
}

bool dma_alloc_stream(unsigned module, unsigned *psi, unsigned mask)
{
	for(unsigned ch = 0; ch < 8; ch ++)
	{
		if (mask & (1 << ch))
		{
			if (__machine_trylock(&_allocated[module][ch], 1))
			{
				*psi = ch;
				return true;
			}
		}
	}
	return false;
}

void dma_free_stream(unsigned module, unsigned ch)
{
	dma_stream_disable(module, ch);
	__machine_unlock(&_allocated[module][ch]);
}

static inline unsigned _disable_stream(unsigned module, unsigned si)
{
	DMA_TypeDef *dma = _modules[module];
	DMA_Stream_TypeDef *s = _streams[module][si];

	while(s->CR & DMAS_CR_EN)
		s->CR = 0;

	unsigned shift = ((si & 1) ? 6 : 0) + ((si & 2) ? 16 : 0);
	unsigned isr = ((si >= 4 ? dma->HISR : dma->LISR) >> shift) & 0x3d;
	if (si >= 4)
		dma->HIFCR = isr << shift;
	else
		dma->LIFCR = isr << shift;
	return isr;
}

void dma_stream_enable(unsigned module, unsigned si, const void *src_ptr, void *dst_ptr, unsigned length, 
	const dma_config_t *config, event_t *done)
{
	ASSERT(module < 2, KERNEL_ERROR_KERNEL_PANIC);
	ASSERT(si < 8, KERNEL_ERROR_KERNEL_PANIC);
	DMA_Stream_TypeDef *s = _streams[module][si];

//1. If the stream is enabled, disable it by resetting the EN bit in the DMA_SxCR register,
//then read this bit in order to confirm that there is no ongoing stream operation. Writing
//this bit to 0 is not immediately effective since it is actually written to 0 once all the
//current transfers have finished. When the EN bit is read as 0, this means that the
//stream is ready to be configured. It is therefore necessary to wait for the EN bit to be
//cleared before starting any stream configuration. All the stream dedicated bits set in the
//status register (DMA_LISR and DMA_HISR) from the previous data block DMA
//transfer should be cleared before the stream can be re-enabled.
	_disable_stream(module, si);

//2. Set the peripheral port register address in the DMA_SxPAR register. The data will be
//moved from/ to this address to/ from the peripheral port after the peripheral event.	
//3. Set the memory address in the DMA_SxMA0R register (and in the DMA_SxMA1R
//register in the case of a double buffer mode). The data will be written to or read from
//this memory after the peripheral event.
	unsigned flow;
	switch(config->Flow & 3)
	{
		case DMA_FLOW_P2M:
			s->PAR = (unsigned)src_ptr;
			s->M0AR = (unsigned)dst_ptr;
			flow = DMAS_CR_DIR_P2M; 
			break;
		case DMA_FLOW_M2P:
			s->M0AR = (unsigned)src_ptr;
			s->PAR = (unsigned)dst_ptr;
			flow = DMAS_CR_DIR_M2P; 
			break;
		default:
			kernel_panic(KERNEL_ERROR_KERNEL_PANIC);
	}

//4. Configure the total number of data items to be transferred in the DMA_SxNDTR
//register. After each peripheral event or each beat of the burst, this value is
//decremented.
	s->NDTR = length;
	if (length == 0) flow |= DMAS_CR_PFCTRL;

//5. Select the DMA channel (request) using CHSEL[2:0] in the DMA_SxCR register.
//6. If the peripheral is intended to be the flow controller and if it supports this feature, set
//the PFCTRL bit in the DMA_SxCR register.
//7. Configure the stream priority using the PL[1:0] bits in the DMA_SxCR register.
//8. Configure the FIFO usage (enable or disable, threshold in transmission and reception)
//9. Configure the data transfer direction, peripheral and memory incremented/fixed mode,
//single or burst transactions, peripheral and memory data widths, Circular mode,
//Double buffer mode and interrupts after half and/or full transfer, and/or errors in the
//DMA_SxCR register.
//10. Activate the stream by setting the EN bit in the DMA_SxCR register.
	s->FCR = (config->Flow & DMA_FLOW_DISABLE_FIFO) ? 0 : DMAS_FCR_DMDIS; // TODO: adjust FIFO
	s->CR = ((config->Channel & 7) << DMAS_CR_CHSEL_BIT) | flow |
		((config->Mem.Burst & 3) << DMAS_CR_MBURST_BIT) | ((config->Periph.Burst & 3) << DMAS_CR_PBURST_BIT) |
		((config->Mem.Width & 3) << DMAS_CR_MSIZE_BIT) | ((config->Periph.Width & 3) << DMAS_CR_PSIZE_BIT) |
		(config->Mem.Increment ? DMAS_CR_MINC : 0) | (config->Periph.Increment ? DMAS_CR_PINC : 0) |
		((config->Flow & DMA_FLOW_CIRCULAR) ? DMAS_CR_CIRC : 0) |
		DMAS_CR_TCIE | DMAS_CR_TEIE | DMAS_CR_DMEIE | DMAS_CR_EN;

	_events[module][si] = done;
	unsigned irqn = _irqn[module][si];
	NVIC_EnableIRQ(irqn);
}

void dma_stream_disable(unsigned module, unsigned si)
{
	_disable_stream(module, si);

	unsigned irqn = _irqn[module][si];
	NVIC_DisableIRQ(irqn);

	_events[module][si] = nullptr;
}

static inline void _irq_handler(unsigned module, unsigned si)
{
	_disable_stream(module, si);
	event_t *event = _events[module][si];
	if (event != nullptr)
		exos_event_set(event);
}

void DMA1_Stream0_IRQHandler() { _irq_handler(0, 0); }
void DMA1_Stream1_IRQHandler() { _irq_handler(0, 1); }
void DMA1_Stream2_IRQHandler() { _irq_handler(0, 2); }
void DMA1_Stream3_IRQHandler() { _irq_handler(0, 3); }
void DMA1_Stream4_IRQHandler() { _irq_handler(0, 4); }
void DMA1_Stream5_IRQHandler() { _irq_handler(0, 5); }
void DMA1_Stream6_IRQHandler() { _irq_handler(0, 6); }
void DMA1_Stream7_IRQHandler() { _irq_handler(0, 7); }
void DMA2_Stream0_IRQHandler() { _irq_handler(1, 0); }
void DMA2_Stream1_IRQHandler() { _irq_handler(1, 1); }
void DMA2_Stream2_IRQHandler() { _irq_handler(1, 2); }
void DMA2_Stream3_IRQHandler() { _irq_handler(1, 3); }
void DMA2_Stream4_IRQHandler() { _irq_handler(1, 4); }
void DMA2_Stream5_IRQHandler() { _irq_handler(1, 5); }
void DMA2_Stream6_IRQHandler() { _irq_handler(1, 6); }
void DMA2_Stream7_IRQHandler() { _irq_handler(1, 7); }

