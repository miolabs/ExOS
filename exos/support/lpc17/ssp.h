#ifndef LPC17_SSP_H
#define LPC17_SSP_H

#define SSP_MODULE_COUNT 2

// control register 0 bits
#define SSPCR0_DSS_BIT 0
#define SSPCR0_FRF_BIT 4
#define SSPCR0_CLKMODE_BIT 6
#define SSPCR0_SCR_BIT 8

#define SSP_DSS_4BIT	3
#define SSP_DSS_5BIT	4
#define SSP_DSS_6BIT	5
#define SSP_DSS_7BIT	6
#define SSP_DSS_8BIT	7
#define SSP_DSS_9BIT	8
#define SSP_DSS_10BIT	9
#define SSP_DSS_11BIT	10
#define SSP_DSS_12BIT	11
#define SSP_DSS_13BIT	12
#define SSP_DSS_14BIT	13
#define SSP_DSS_15BIT	14
#define SSP_DSS_16BIT	15

#define SSP_FRF_SPI			0
#define SSP_FRF_TI			1
#define SSP_FRF_Microwire	2

typedef enum 
{
	SSP_CLK_POL0_PHA0 = 0,
	SSP_CLK_POL1_PHA0 =	1,
	SSP_CLK_POL0_PHA1 = 2,
	SSP_CLK_POL1_PHA1 = 3
} SSP_CLK_MODE;

// control register 1 bits
#define SSPCR1_LBM (1<<0)
#define SSPCR1_SSE (1<<1)
#define SSPCR1_MS (1<<2)
#define SSPCR1_SOD (1<<3)

// status register bits
typedef struct
{
	unsigned TFE:1;		// Transmit FIFO Empty
	unsigned TNF:1;		// Transmit FIFO Not Full
	unsigned RNE:1;		// Receive FIFO Not Empty
	unsigned RFF:1;		// Receive FIFO Full
	unsigned BSY:1;		// Busy
} _SSPSRbits;

// interrupt mask set/clear and request registers
typedef struct
{
	unsigned ROR:1;		// Receive OverRun
	unsigned RT:1;		// Receive Timeout
	unsigned RX:1;		// Rx FIFO (is at least half full)
	unsigned TX:1;		// Tx FIFO (is at least half empty)
} _SSPINTbits;

// dma control register
typedef struct
{
	unsigned RXDMAE:1;	// Enable DMA for receive FIFO
	unsigned TXDMAE:1;	// Enable DMA for transmit FIFO
} _SSPDMACbits;

#define SSP_DMA_RXDMAE (1<<0)
#define SSP_DMA_TXDMAE (1<<1)

typedef struct
{
	volatile unsigned long CR0;	// Control Register 0
	volatile unsigned long CR1;	// Control Register 1

	volatile unsigned long DR;	// Data Register

	union
	{
		volatile unsigned long SR;
		volatile _SSPSRbits SRbits;
	};	// Status Register

	volatile unsigned long CPSR;	// Clock Prescaler

    volatile unsigned long IMSC;	// Interrupt Mask Set/Clear
    union
	{
		volatile unsigned long RIS;
		volatile _SSPINTbits RISbits;
	};	// Raw Interrupt Status
    union
	{
		volatile unsigned long MIS;
		volatile _SSPINTbits MISbits;
	};	// Masked Interrupt Status
    volatile unsigned long ICR;	// Interrupt Clear Register (for ROR and RT bis only, write-only)
    union
	{
		volatile unsigned long DMACR;
		volatile _SSPDMACbits DMACRbits;
	};	// DMA Control Register
} SSP_MODULE;

// hooks
void ssp_dma_reset_event(int module) __attribute__((__weak__));
void ssp_dma_set_event(int module) __attribute__((__weak__));
void ssp_dma_wait(int module) __attribute__((__weak__));

#endif // LPC17_SPP_H
