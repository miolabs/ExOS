#ifndef LPC2K_MCI_H
#define LPC2K_MCI_H

#include <support/misc/sdcard/sdcard_mci.h>

// control register 0 bits
typedef struct
{
	unsigned Control:2;
	unsigned Reserved:4;
	unsigned OpenDrain:1;	// CMD output control
	unsigned Rod:1;
} _MCIPowerBits;

#define MCIPower (*(volatile unsigned long *)0xE008C000)
#define MCIPowerBits (*(volatile _MCIPowerBits *)&MCIPower)
#define MCI_POWER_OFF	0
#define MCI_POWER_UP	2
#define MCI_POWER_ON	3

// clock control register
typedef struct
{
	unsigned ClkDiv:8;
	unsigned Enable:1;
	unsigned PwrSave:1;
	unsigned Bypass:1;
	unsigned WideBus:1;
} _MCIClockBits;

#define MCIClock (*(volatile unsigned long *)0xE008C004)
#define MCIClock_ClkDiv_MASK 0x000000FF
#define MCIClock_Enable (1<<8)
#define MCIClock_PwrSave (1<<9)
#define MCIClock_Bypass (1<<10)
#define MCIClock_WideBus (1<<11)

#define MCIArgument (*(volatile unsigned long *)0xE008C008)
#define MCICommand (*(volatile unsigned long *)0xE008C00C)
#define MCICommand_Response		0x0040
#define MCICommand_LongRsp		0x0080
#define MCICommand_Interrupt	0x0100
#define MCICommand_Pending		0x0200
#define MCICommand_Enable		0x0400

#define MCIRespCmd (*(volatile unsigned long *)0xE008C010)
#define MCIResponse0 (*(volatile unsigned long *)0xE008C014)
#define MCIResponse1 (*(volatile unsigned long *)0xE008C018)
#define MCIResponse2 (*(volatile unsigned long *)0xE008C01C)
#define MCIResponse3 (*(volatile unsigned long *)0xE008C020)
#define MCIDataTimer (*(volatile unsigned long *)0xE008C024)
#define MCIDataLength (*(volatile unsigned long *)0xE008C028)
#define MCIDataCtrl (*(volatile unsigned long *)0xE008C02C)
#define MCIDataCnt (*(volatile unsigned long *)0xE008C030)

#define MCIDataCtrl_Enable		1
#define MCIDataCtrl_Direction	2
#define MCIDataCtrl_Mode		4
#define MCIDataCtrl_DMAEnable	8
#define MCIDataCtrl_BlockSize_BIT	4
#define MCIDataCtrl_BlockSize_MASK	0xF0

#define MCI_DATACTRL_ENABLE_READ	(MCIDataCtrl_Enable | MCIDataCtrl_Direction)
#define MCI_DATACTRL_ENABLE_WRITE	(MCIDataCtrl_Enable)

// status register
typedef struct
{
	unsigned CmdCrcFail:1;
	unsigned DataCrcFail:1;
	unsigned CmdTimeout:1;
	unsigned DataTimeout:1;
	unsigned TxUnderrun:1;
	unsigned RxOverrun:1;
	unsigned CmdRespEnd:1;		// response received (CRC ok)
	unsigned CmdSent:1;			// command send (no response received)
	unsigned DataEnd:1;			// counter reached zero
	unsigned StartBitErr:1;		// start bit not detected on all data lines (wide bus mode)
	unsigned DataBlockEnd:1;	// Data block sent/received (CRC ok)

	unsigned CmdActive:1;		// Command transfer in progress
	unsigned TxActive:1;		// Data transmit in progress
	unsigned RxActive:1;		// Data receive in progress
	unsigned TxFifoHalfEmpty:1;
	unsigned RxFifoHalfFull:1;
	unsigned TxFifoFull:1;
	unsigned RxFifoFull:1;
	unsigned TxFifoEmpty:1;
	unsigned RxFifoEmpty:1;
	unsigned TxDataAvailable:1;
	unsigned RxDataAvailable:1;
} MCI_STATUS_BITS;

#define MCI_STATUS_TERM_MASK 0x7FF	// bits in MCIStatus/MCIClear/MCIMask0 int a (normal) cmd termination

#define MCIStatus (*(volatile unsigned long *)0xE008C034)
#define MCIStatusBits (*(volatile _MCIStatusBits *)&MCIStatus)

#define MCIClear (*(volatile unsigned long *)0xE008C038)
#define MCIMask0 (*(volatile unsigned long *)0xE008C03C)
#define MCIMask0Bits (*(volatile _MCIStatusBits *)&MCIMask0)
#define MCIFifoCnt (*(volatile unsigned long *)0xE008C048)
#define MCIFIFO (*(volatile unsigned long *)0xE008C080)


#endif // LPC2K_MCI_H


