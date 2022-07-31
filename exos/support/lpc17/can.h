#ifndef LPC17_CAN_H
#define LPC17_CAN_H

#include <support/can_hal.h>

// frame register bits
typedef struct
{
	unsigned IDIndex:10;
	unsigned BP:1;
	unsigned :5;
	unsigned DLC:4;
	unsigned :10;
	unsigned RTR:1;
	unsigned FF:1;
} _CANRFSbits;

// interrupt and capture bits
typedef struct
{
	unsigned RI:1;
	unsigned TI1:1;
	unsigned EI:1;
	unsigned DOI:1;
	unsigned WUI:1;
	unsigned EPI:1;
	unsigned ALI:1;
	unsigned BEI:1;
	unsigned IDI:1;
	unsigned TI2:1;
	unsigned TI3:1;
	unsigned ERRBIT:5;
	unsigned ERRDIR:1;
	unsigned ERRC:2;
	unsigned ALCBIT:5;
} _CANICRbits;

// Interrupt (ICR/IE) bits
#define CAN_RI		0x0001
#define CAN_TI1		0x0002
#define CAN_EI		0x0004
#define CAN_DOI		0x0008
#define CAN_WUI		0x0010
#define CAN_EPI		0x0020
#define CAN_ALI		0x0040
#define CAN_BEI		0x0080
#define CAN_IDI		0x0100
#define CAN_TI2		0x0200
#define CAN_TI3		0x0400

// global status bits
typedef struct
{
	unsigned RBS:1;
	unsigned DOS:1;
	unsigned TBS:1;
	unsigned TCS:1;
	unsigned RS:1;
	unsigned TS:1;
	unsigned ES:1;
	unsigned BS:1;
	unsigned reserved:8;
	unsigned RXERR:8;
	unsigned TXERR:8;
} _CANGSRbits;

// Command Requests
#define CAN_CMR_TR		0x01	// Transmit Request
#define CAN_CMR_AT		0x02	// Abort Transmission
#define CAN_CMR_RRB		0x04	// Release Receive Buffer
#define CAN_CMR_CDO		0x08	// Clear Data Overrun
#define CAN_CMR_SRR		0x10	// Self Reception Request
#define CAN_CMR_STB1	0x20	// Select Tx Buffer 1
#define CAN_CMR_STB2	0x40	// Select Tx Buffer 2
#define CAN_CMR_STB3	0x80	// Select Tx Buffer 3
#define CAN_CMR_TR_ONCE (CAN_CMR_TR | CAN_CMR_AT) // Transmit once (no retransmission on error)
#define CAN_CMR_SRR_ONCE (CAN_CMR_SRR | CAN_CMR_TR) // Self receive once (no retransmission on error)

typedef struct
{
	volatile unsigned char TB1;
	volatile unsigned char TB2;
	volatile unsigned char TB3;
	volatile unsigned char Reserved;
} _CANSR;

// Status bits (per byte)
#define CAN_SR_RBS	0x01	// Receive Buffer Status*
#define CAN_SR_DOS	0x02	// Data Overrun Status*
#define CAN_SR_TBS	0x04	// Transmit Buffer Status
#define CAN_SR_TCS	0x08	// Transmit Complete Status
#define CAN_SR_RS	0x10	// Receive (Busy) Status*
#define CAN_SR_TS	0x20	// Transmit (Busy) Status
#define CAN_SR_ES	0x40	// Error Status*
#define CAN_SR_BS	0x80	// Bus (Off) Status*
// * bits are common to single receive buffer and equal to those in GSR register 

typedef struct
{
    volatile unsigned long TFI;
    volatile unsigned long TID;
    volatile unsigned long TDA;
    volatile unsigned long TDB;
} CAN_TB;	// Transmit buffer

typedef struct
{
	volatile unsigned long MOD;
	volatile unsigned long CMR;
	union
	{
		volatile unsigned long GSR;
		volatile _CANGSRbits GSRbits;
	};
	union
	{
		volatile unsigned long ICR;
		volatile _CANICRbits ICRbits;
	};
	volatile unsigned long IER;
	volatile unsigned long BTR;
	volatile unsigned long EWL;
	union
	{
		volatile unsigned long SR;
		volatile _CANSR SRbits;
	};
	union
	{
		volatile unsigned long RFS;
		volatile _CANRFSbits RFSbits;
	};
    volatile unsigned long RID;
    volatile unsigned long RDA;
    volatile unsigned long RDB;
	CAN_TB TB1;
	CAN_TB TB2;
	CAN_TB TB3;
} CAN_MODULE;

typedef struct
{
	union
	{
        unsigned long AFMR;
		struct
		{
			unsigned AccOff:1;
			unsigned AccBP:1;
			unsigned eFCAN:1;
		} AFMRbits;
	};
	unsigned long SFF_sa;
	unsigned long SFF_GRP_sa;
	unsigned long EFF_sa;
	unsigned long EFF_GRP_sa;
	unsigned long ENDofTable;
	volatile unsigned long LUTerrAd;
	volatile unsigned long LUTerr;
	volatile unsigned long FCANIE;
	volatile unsigned long FCANIC0;
	volatile unsigned long FCANIC1;
} FCAN_MODULE;

#define AFMR_AccOff	(1<<0)
#define AFMR_AccBP	(1<<1)
#define AFMR_eFCAN	(1<<2)

#define FCAN_SFF_DISABLE (1<<12)
#define FCAN_SFF_INTEN (1<<11)
#define FCAN_SFF_MASK (FCAN_SFF_DISABLE|FCAN_SFF_INTEN)
#define FCAN_MAKE_SFF(scc, id, flags) (((scc) << 13) | ((id) & 0x7FF) | ((flags) & FCAN_SFF_MASK))

typedef struct
{
	union
	{
		volatile unsigned long Status;
		volatile struct
		{
			unsigned ID:11;
			unsigned :2;
			unsigned SCC:3;
			unsigned DLC:4;
			unsigned :4;
			unsigned SEM:2;
			unsigned MsgLost:1;
			unsigned :3;
			unsigned RTR:1;
			unsigned FF:1;
		} StatusBits;
	};
	volatile CAN_BUFFER Data;
} FCAN_MSG;

#define FCAN_MSG_SCC_BIT 13
#define FCAN_MSG_DLC_BIT 16
#define FCAN_MSG_SEM_BIT 24
#define FCAN_MSG_SEM_MASK (3<<FCAN_MSG_SEM_BIT)

typedef enum
{
	CAN_TB1 = 1,
	CAN_TB2 = 2,
	CAN_TB3 = 3,
} CAN_TBUF;

// prototypes
void can_flush(int module);
void can_reset(int module);
unsigned long can_get_reset_count(int module);
unsigned long can_get_error_count(int module);

#endif // LPC17_CAN_H
