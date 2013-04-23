#ifndef LPC11_SSP_H
#define LPC11_SSP_H

struct _SSP_CR0
{
	unsigned DSS:4;
	unsigned FRF:2;
	unsigned CPOL:1;
	unsigned CPHA:1;
	unsigned SCR:8;
};

typedef enum
{
	SSP_CR0_CPOL = (1<<6),
	SSP_CR0_CPHA = (1<<7),
} SSP_CR0;

struct _SSP_CR1
{
	unsigned LBM:1;
	unsigned SSE:1;
	unsigned MS:1;
	unsigned SOD:1;
};

typedef enum
{
	SSP_CR1_LBM = 0x1,
	SSP_CR1_SSE = 0x2,
	SSP_CR1_MS = 0x4,
	SSP_CR1_SOD = 0x8,
} SSP_CR1;

struct _SSP_SR
{
	unsigned TFE:1;
	unsigned TNF:1;
	unsigned RNE:1;
	unsigned RFF:1;
	unsigned BSY:1;
};

struct _SSP_IM
{
	unsigned ROR:1;
	unsigned RT:1;
	unsigned RX:1;
	unsigned TX:1;
};

typedef enum
{
	SSP_IM_ROR = (1<<0),
	SSP_IM_RT = (1<<1),
	SSP_IM_RX = (1<<2),
	SSP_IM_TX = (1<<3),
} SSP_IM;

typedef struct _SSP
{
	union
	{
		struct _SSP_CR0 Bits;
		unsigned long Value;
	} CR0;
	union
	{
		struct _SSP_CR1 Bits;
		unsigned long Value;
	} CR1;
	unsigned long DATA;
	union
	{
		struct _SSP_SR Bits;
		unsigned long Value;
	} SR;
	unsigned long CPSR;
	union
	{
		struct _SSP_IM Bits;
		unsigned long Value;
	} IMSC;
	union
	{
		struct _SSP_IM Bits;
		unsigned long Value;
	} RIS;
	union
	{
		struct _SSP_IM Bits;
		unsigned long Value;
	} MIS;
	union
	{
		struct _SSP_IM Bits;
		unsigned long Value;
	} ICR;
} SSP_MODULE;


// prototypes
SSP_MODULE *ssp_initialize(int module, int freq, int bit_width);


#endif // LPC11_SSP_H
