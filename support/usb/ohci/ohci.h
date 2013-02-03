#ifndef OHCI_H
#define OHCI_H

#include <usb/usb.h>

// Root Hub Downstream Ports
#define USB_HOST_ROOT_HUB_NDP 2

typedef enum
{
	OHCI_OP_CONTROL_FS_RESET = 0,
	OHCI_OP_CONTROL_FS_RESUME = 1,
	OHCI_OP_CONTROL_FS_OPERATIONAL = 2,
	OHCI_OP_CONTROL_FS_SUSPEND = 3,
} OHCI_OP_CONTROL_FS;

typedef struct _OHCI_OP_INTERRUPTS
{
	unsigned SO:1;		// Scheduling Overrun
	unsigned WDH:1;		// Writeback Done Head
	unsigned SF:1;		// Start of Frame
	unsigned RD:1;		// Resume Detected
	unsigned UE:1;		// Unrecoverable Error
	unsigned FNO:1;		// Frame Number Overflow
	unsigned RHSC:1;	// Roothub Status Change
	unsigned :23;
	unsigned OC:1;		// Ownership Changed
	unsigned MIE:1;		// Master Interrupt Enable
} OHCI_OP_INTERRUPTS;


// OHCI Host Controller Operational Registers
typedef struct
{
	volatile unsigned long Revision;
	union
	{
		volatile unsigned long Control;
		volatile struct
		{
			unsigned CBSR:2;	// Control-Bulk Service Ratio
			unsigned PLE:1;		// Periodic List Enable
			unsigned IE:1;		// Isochronous Enable
			unsigned CLE:1;		// Control List Enable
			unsigned BLE:1;		// Bulk List Enable
			OHCI_OP_CONTROL_FS HCFS:2;	// Functional State
			unsigned IR:1;		// Interrupt Routed
			unsigned RWC:1;		// Remote Wakeup Connected
			unsigned RWE:1;		// Remote Wakeup Enable 
		} ControlBits;
	};
	volatile unsigned long CommandStatus;
	union
	{
		volatile unsigned long InterruptStatus;
		volatile OHCI_OP_INTERRUPTS InterruptStatusBits;
	};
	union
	{
		volatile unsigned long InterruptEnable;
        volatile OHCI_OP_INTERRUPTS InterruptEnableBits;
	};
	union
	{
		volatile unsigned long InterruptDisable;
        volatile OHCI_OP_INTERRUPTS InterruptDisableBits;
	};
	volatile void *HCCA;
	volatile unsigned long PeriodCurrentED;
	volatile void *ControlHeadED;
	volatile void *ControlCurrentED;
	volatile void *BulkHeadED;
	volatile void *BulkCurrentED;
	volatile unsigned long DoneHead;

	volatile unsigned long FmInterval;
	volatile unsigned long FmRemaining;
	volatile unsigned long FmNumber;
	volatile unsigned long PeriodicStart;
	volatile unsigned long LSThreshold;

	volatile unsigned long RhDescriptorA;
	volatile unsigned long RhDescriptorB;
	volatile unsigned long RhStatus;
	volatile unsigned long RhPortStatus[USB_HOST_ROOT_HUB_NDP];
} OHCI_OP_REGISTERS;

// HcControl Register
#define  OHCIR_CONTROL_PLE		0x00000004
#define  OHCIR_CONTROL_IE		0x00000008
#define  OHCIR_CONTROL_CLE		0x00000010
#define  OHCIR_CONTROL_BLE		0x00000020
#define  OHCIR_CONTROL_HCFS		0x000000C0
#define  OHCIR_CONTROL_HC_OPER	0x00000080

// HcCommandStatus Register
#define  OHCIR_CMD_STATUS_HCR              0x00000001
#define  OHCIR_CMD_STATUS_CLF              0x00000002
#define  OHCIR_CMD_STATUS_BLF              0x00000004

// HcInterruptStatus Register
#define  OHCIR_INTR_STATUS_SO		0x00000001
#define  OHCIR_INTR_STATUS_WDH		0x00000002
#define  OHCIR_INTR_STATUS_UE		0x00000010
#define  OHCIR_INTR_STATUS_RHSC		0x00000040

// HcInterruptEnable Register
#define  OHCIR_INTR_ENABLE_SO		0x00000001
#define  OHCIR_INTR_ENABLE_WDH		0x00000002
#define  OHCIR_INTR_ENABLE_UE		0x00000010
#define  OHCIR_INTR_ENABLE_RHSC		0x00000040
#define  OHCIR_INTR_ENABLE_MIE		0x80000000

// HcRhStatus Register
#define  OHCIR_RH_STATUS_LPS		0x00000001
#define  OHCIR_RH_STATUS_LPSC		0x00010000

// HcRhPortStatus Registers
#define OHCIR_RH_PORT_CCS	0x00000001
#define OHCIR_RH_PORT_PRS	0x00000010
#define	OHCIR_RH_PORT_LSDA	0x00000200
#define OHCIR_RH_PORT_CSC	0x00010000
#define OHCIR_RH_PORT_PRSC	0x00100000

typedef enum
{
	OHCI_TD_SETUP = 0,
	OHCI_TD_DIR_OUT = 1,
	OHCI_TD_DIR_IN = 2,
	OHCI_TD_RESERVED = 3,
} OHCI_TD_PID;

typedef enum
{
	OHCI_TD_TOGGLE_CARRY = 0,
	OHCI_TD_TOGGLE_0 = 2,
	OHCI_TD_TOGGLE_1 = 3,
} OHCI_TD_TOGGLE;

typedef enum
{
	OHCI_TD_CC_NO_ERROR = 0,
	OHCI_TD_CC_CRC = 1,
	OHCI_TD_CC_BIT_STUFFING = 2,
	OHCI_TD_CC_DATA_TOGGLE_MISMATCH = 3,
	OHCI_TD_CC_STALL = 4,
	OHCI_TD_CC_NOT_RESPONDING = 5,
	OHCI_TD_CC_PIDCHECK = 6,
	OHCI_TD_CC_UNEXPECTED_PID = 7,
	OHCI_TD_CC_DATA_OVERRUN = 8,
	OHCI_TD_CC_DATA_UNDERRUN = 9,
	OHCI_TD_CC_BUFFER_OVERRUN = 12,
	OHCI_TD_CC_BUFFER_UNDERRUN = 13,
	OHCI_TD_CC_NOT_ACCESSED = 14,
} OHCI_TD_CC;

//  Host Controller Transfer Descriptor
typedef struct __attribute__((aligned(16))) _OHCI_HCTD
{
	union
	{
		volatile int Control;
		struct
		{
			unsigned :18;
			unsigned Rounding:1;
			OHCI_TD_PID Pid:2;
			unsigned DelayInterrupt:3;
			OHCI_TD_TOGGLE Toggle:2;
			unsigned ErrorCount:2;
			OHCI_TD_CC ConditionCode:4;
		} ControlBits;
	};
    volatile void *CurrBufPtr;
    volatile struct _OHCI_HCTD *Next;
    volatile void *BufferEnd;
} OHCI_HCTD;

typedef struct __attribute__((aligned(32))) _OHCI_HCTD_ISO
{
	union
	{
		volatile int Control;
		struct
		{
			unsigned StartingFrame:16;
			unsigned :5;
			unsigned DelayInterrupt:3;
			unsigned FrameCount:3;
			unsigned :1;
			OHCI_TD_CC ConditionCode:4;
		} ControlBits;
	};
    volatile void *BP0;
    volatile struct _OHCI_HCTD_ISO *Next;
    volatile void *BufferEnd;
	union
	{
		unsigned short PSW[8];
		unsigned short Offset[8];
	};
} OHCI_HCTD_ISO;

typedef enum
{
	OHCI_ED_DIR_FROM_TD = 0,
	OHCI_ED_DIR_OUT = 1,
	OHCI_ED_DIR_IN = 2,
	OHCI_ED_DIR_FROM_TD_3 = 3,
} OHCI_ED_DIRECTION;

// Host Controller EndPoint Descriptor
typedef struct _OHCI_HCED
{
	union
	{
		volatile int Control;
		volatile struct
		{
			unsigned FunctionAddress:7;
			unsigned Endpoint:4;
			OHCI_ED_DIRECTION Direction:2;
			unsigned Speed:1;
			unsigned sKip:1;
			unsigned Format:1;
			unsigned MaxPacketSize:11;
		} ControlBits;
	};
    volatile OHCI_HCTD *TailTD;
	union
	{
		volatile OHCI_HCTD *HeadTD;
		volatile struct
		{
			unsigned Halted:1;
			unsigned toggleCarry:1;
			unsigned :2;
			unsigned Ptr16:28;
		} HeadTDBits;
	};
    volatile struct _OHCI_HCED *Next;
} OHCI_HCED;

// Host Controller Communication Area
typedef struct __attribute__((aligned(256))) _OHCI_HCCA 
{
	volatile OHCI_HCED *IntTable[32];
    volatile int FrameNumber;
    volatile OHCI_HCTD *DoneHead;
    volatile unsigned char Reserved[116];
    volatile unsigned char Unknown[4];	// Unused
} OHCI_HCCA;

extern OHCI_OP_REGISTERS *_hc;

// prototypes
int ohci_initialize();
OHCI_HCED **ohci_get_periodic_ep(int index);
void ohci_clear_hced(OHCI_HCED *hced);
void ohci_clear_hctd(OHCI_HCTD *hctd);
void ohci_host_clear_hctd_iso(OHCI_HCTD_ISO *isotd);
int ohci_init_hctd_iso(OHCI_HCTD_ISO *itd, int sf, void *buffer, int length, int packet_size);

unsigned short ohci_get_current_frame();

#endif // OHCI_H
