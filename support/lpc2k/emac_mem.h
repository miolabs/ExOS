#ifndef LPC_EMAC24_MEM_H
#define LPC_EMAC24_MEM_H

typedef struct _ETH_RX_DESC
{
	void *Data;
	struct
	{
		unsigned Size:11;
		unsigned :20;
		unsigned Interrupt:1;
	} ControlBits;
} ETH_RX_DESC;

typedef struct _ETH_RX_STATUS
{
	union
	{
		struct
		{
			unsigned Size:11;
			unsigned :7;
			unsigned ControlFrame:1;
			unsigned VLAN:1;
			unsigned FailFilter:1;
			unsigned Multicast:1;
			unsigned Broadcast:1;
			unsigned CRCError:1;
			unsigned SymbolError:1;
			unsigned LengthError:1;
			unsigned RangeError:1;
			unsigned AlignmentError:1;
			unsigned Overrun:1;
			unsigned NoDescriptor:1;
			unsigned LastFlag:1;
			unsigned Error:1;
		} StatusBits;
		unsigned long Status;
	};
	struct
	{
		unsigned short SAHashCRC;
		unsigned short DAHashCRC;
	} StatusHashCRC;
} ETH_RX_STATUS;

typedef struct _ETH_TX_DESC
{
	void *Data;
	union
	{
		struct _ETH_TX_CONTROL_BITS
		{
			unsigned Size:11;
			unsigned :15;
			unsigned Override:1;
			unsigned Huge:1;
			unsigned Pad:1;
			unsigned CRC:1;
			unsigned Last:1;
			unsigned Interrupt:1;
		} ControlBits;
		unsigned long Control;
	};
} ETH_TX_DESC;

#define ETH_TX_DESC_CONTROL_SIZE_MASK 0x7ff
#define ETH_TX_DESC_CONTROL_OVERRIDE (1<<26)
#define ETH_TX_DESC_CONTROL_HUGE (1<<27)
#define ETH_TX_DESC_CONTROL_PAD (1<<28)
#define ETH_TX_DESC_CONTROL_CRC (1<<29)
#define ETH_TX_DESC_CONTROL_LAST (1<<30)
#define ETH_TX_DESC_CONTROL_INTERRUPT (1<<31)

typedef struct _ETH_TX_STATUS
{
	union
	{
		struct 
		{
			unsigned :21;
			unsigned CollisionCount:4;
			unsigned Defer:1;
			unsigned ExcessiveDefer:1;
			unsigned ExcessiveCollision:1;
			unsigned LateCollision:1;
			unsigned Underrun:1;
			unsigned NoDecriptor:1;
			unsigned Error:1;
		} StatusBits;
		unsigned long Status;
	};
} ETH_TX_STATUS;

// prototypes
void emac_mem_initialize();
void emac_mem_tx_handler();

#endif // LPC_EMAC24_MEM_H
