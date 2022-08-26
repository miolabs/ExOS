#ifndef STM32F4_DMA_H
#define STM32F4_DMA_H

#include <stdbool.h>
#include <kernel/event.h>

typedef enum
{
	DMA_BURST_1 = 0,
	DMA_BURST_4 = 1,
	DMA_BURST_8 = 2,
	DMA_BURST_16 = 3,
} dma_burst_t;

typedef enum
{
	DMA_WIDTH_8BIT = 0,
	DMA_WIDTH_16BIT = 1,
	DMA_WIDTH_32BIT = 2,
} dma_width_t;

typedef enum
{
	DMA_FLOW_P2M = 0,
	DMA_FLOW_M2P = 1,
	DMA_FLOW_M2M = 2,
	DMA_FLOW_CIRCULAR = 1<<5,
	DMA_FLOW_DISABLE_FIFO = 1<<6,
} dma_flow_t;

typedef enum
{
	DMA_CHF_0 = (1<<0),
	DMA_CHF_1 = (1<<1),
	DMA_CHF_2 = (1<<2),
	DMA_CHF_3 = (1<<3),
	DMA_CHF_4 = (1<<4),
	DMA_CHF_5 = (1<<5),
	DMA_CHF_6 = (1<<6),
	DMA_CHF_7 = (1<<7),
} dma_ch_t;

typedef struct __attribute__((__packed__))
{
	unsigned Increment:1;
	dma_burst_t Burst:2;
	dma_width_t Width:3;
} dma_con_t;

typedef struct __attribute__((__packed__))
{
	dma_con_t Mem;
	dma_con_t Periph;
	dma_flow_t Flow;
	unsigned char Channel;
} dma_config_t;

#define DMAS_CR_CHSEL_BIT	25
#define DMAS_CR_MBURST_BIT	23
#define DMAS_CR_PBURST_BIT	21
#define DMAS_CR_PL_BIT		16
#define DMAS_CR_PINCOS	(1<<15)
#define DMAS_CR_MSIZE_BIT	13
#define DMAS_CR_PSIZE_BIT	11
#define DMAS_CR_MINC	(1<<10)
#define DMAS_CR_PINC	(1<<9)
#define DMAS_CR_CIRC	(1<<8)
#define DMAS_CR_DIR_P2M	(0<<6)
#define DMAS_CR_DIR_M2P	(1<<6)
#define DMAS_CR_DIR_M2M	(2<<6)
#define DMAS_CR_PFCTRL	(1<<5)
#define DMAS_CR_TCIE	(1<<4)
#define DMAS_CR_HTIE	(1<<3)
#define DMAS_CR_TEIE	(1<<2)
#define DMAS_CR_DMEIE	(1<<1)
#define DMAS_CR_EN		(1<<0)

#define DMAS_FCR_FEIE	(1<<7)
#define DMAS_FCR_DMDIS	(1<<2)

#define DMA_IS_FEIF		(1<<0)
#define DMA_IS_DMEIF	(1<<2)
#define DMA_IS_TEIF		(1<<3)
#define DMA_IS_HTIF		(1<<4)
#define DMA_IS_TCIF		(1<<5)

// prototypes
void dma_initialize();
bool dma_alloc_stream(unsigned module, unsigned *psi, unsigned mask);
void dma_free_stream(unsigned module, unsigned si);
void dma_stream_enable(unsigned module, unsigned si, const void *src_ptr, void *dst_ptr, unsigned length, 
	const dma_config_t *config, event_t *done);
void dma_stream_disable(unsigned module, unsigned si);

#endif // STM32F4_DMA_H

