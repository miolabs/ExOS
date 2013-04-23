#ifndef LPC17_DMA_H
#define LPC17_DMA_H

#include "cpu.h"

typedef struct                          /* Common Registers                   */
{
	volatile unsigned long IntStat;
	volatile unsigned long IntTCStat;
	volatile unsigned long IntTCClear;
	volatile unsigned long IntErrStat;
	volatile unsigned long IntErrClr;
	volatile unsigned long RawIntTCStat;
	volatile unsigned long RawIntErrStat;
	volatile unsigned long EnbldChns;
	volatile unsigned long SoftBReq;
	volatile unsigned long SoftSReq;
	volatile unsigned long SoftLBReq;
	volatile unsigned long SoftLSReq;
	volatile unsigned long Config;
	volatile unsigned long Sync;
} DMA_MODULE;

#define DMACConfig_E 1

typedef struct
{
	unsigned TransferSize:12;
	unsigned SBSize:3;			// Source Burst
	unsigned DBSize:3;			// Destination Burst
	unsigned SWidth:3;			// Source Width
	unsigned DWidth:3;			// Destination Width
	unsigned Reserved:2;
	unsigned SI:1;
	unsigned DI:1;
	unsigned Prot:3;
	unsigned I:1;				// Terminal count interrupt enable
} _DMACCControlBits;

typedef enum
{
	DMA_BURST_1 = 0,
	DMA_BURST_4 = 1,
	DMA_BURST_8 = 2,
	DMA_BURST_16 = 3,
	DMA_BURST_32 = 4,
	DMA_BURST_64 = 5,
	DMA_BURST_128 = 6,
	DMA_BURST_256 = 7,
} DMA_BURST;

typedef enum
{
	DMA_WIDTH_8BIT = 0,
	DMA_WIDTH_16BIT = 1,
	DMA_WIDTH_32BIT = 2,
} DMA_WIDTH;

typedef struct
{
	unsigned Enable:1;				// Channel enable
	unsigned SrcPeripheral:5;
	unsigned DestPeripheral:5;
	unsigned FlowCtrl:3;
	unsigned IE:1;				// Interrupt error mask
	unsigned ITC:1;				// Terminal Count Interrupt mask 
	unsigned Lock:1;
	unsigned Active:1;
	unsigned Halt:1;
} _DMACCConfigurationBits;

typedef enum
{
	DMA_FLOW_M2M_DMA = 0,
	DMA_FLOW_M2P_DMA = 1,
	DMA_FLOW_P2M_DMA = 2,
	DMA_FLOW_P2P_DMA = 3,
	DMA_FLOW_P2P_DP = 4,
	DMA_FLOW_M2P_DP = 5,
	DMA_FLOW_P2M_SP = 6,
	DMA_FLOW_P2P_SP = 7,
} DMA_FLOW;

typedef enum
{
	DMA_P_SSP0_TX = 0,
	DMA_P_SSP0_RX = 1,
	DMA_P_SSP1_TX = 2,
	DMA_P_SSP1_RX = 3,
	DMA_P_ADC = 4,
	DMA_P_I2S_CH0 = 5,
	DMA_P_I2S_CH1 = 6,
	DMA_P_DAC = 7,
	DMA_P_UART0_TX = 8,
	DMA_P_UART0_RX = 9,
	DMA_P_UART1_TX = 10,
	DMA_P_UART1_RX = 11,
	DMA_P_UART2_TX = 12,
	DMA_P_UART2_RX = 13,
	DMA_P_UART3_TX = 14,
	DMA_P_UART3_RX = 15,
	DMA_P_MAT0_0 = 8,
	DMA_P_MAT0_1 = 9,
	DMA_P_MAT1_0 = 10,
	DMA_P_MAT1_1 = 11,
	DMA_P_MAT2_0 = 12,
	DMA_P_MAT2_1 = 13,
	DMA_P_MAT3_0 = 14,
	DMA_P_MAT3_1 = 15,
} DMA_PERIPHERAL;

typedef struct
{
	volatile unsigned long SrcAddr;
	volatile unsigned long DstAddr;
	volatile unsigned long LLI;
	union
	{
		volatile unsigned long Control;
		volatile _DMACCControlBits ControlBits;
	};
} DMA_TRANSFER;

typedef void (* DMA_CALLBACK)(int channel, int tc_done, void *state);

typedef struct
{
	DMA_TRANSFER Transfer;
	union
	{
		volatile unsigned long Configuration;
		volatile _DMACCConfigurationBits ConfigurationBits;
	};
} DMA_CHANNEL;

#define DMA_CHANNEL_COUNT 8
#define DMA_CHANNEL_MASK ((1<<DMA_CHANNEL_COUNT) - 1)

typedef struct __attribute__((__packed__))
{
	DMA_BURST Burst:3;
	DMA_WIDTH Width:3;
	unsigned Increment:1;
} DMA_CON;

typedef struct __attribute__((__packed__))
{
	DMA_CON Src;
	DMA_CON Dst;
	DMA_PERIPHERAL Peripheral:8;
	DMA_FLOW Flow:8;
} DMA_CONFIG;



// prototypes
void dma_initialize();
int dma_alloc_channels(int *array, int length);
void dma_free_channel(int ch);
void dma_transfer_setup(DMA_TRANSFER *tr, void *src_ptr, void *dst_ptr, int size, 
	const DMA_CONFIG *config, DMA_CALLBACK callback, DMA_TRANSFER *next);
void dma_channel_enable(int ch, const DMA_TRANSFER *tr, const DMA_CONFIG *config, DMA_CALLBACK callback, void *state);
void dma_channel_enable_fast(int ch, void *src_ptr, void *dst_ptr, int size, 
	const DMA_CONFIG *config, DMA_CALLBACK callback, void *state);
void dma_channel_disable(int ch);

#endif // LPC17_DMA_H
