#ifndef C_CAN_H
#define C_CAN_H

#define C_CAN_CMDREQ_BUSY (1<<15)

typedef enum
{
	C_CAN_CMDMSK_DATA_B = (1<<0),
	C_CAN_CMDMSK_DATA_A = (1<<1),
	C_CAN_CMDMSK_TXRQST = (1<<2),	// on writes only
	C_CAN_CMDMSK_NEWDAT = (1<<2),	// on reads only
	C_CAN_CMDMSK_CLRINTPND = (1<<3),	// on reads only
	C_CAN_CMDMSK_CTRL = (1<<4),
	C_CAN_CMDMSK_ARB = (1<<5),
	C_CAN_CMDMSK_MASK = (1<<6),
	C_CAN_CMDMSK_WR = (1<<7),
} C_CAN_CMDMSK_ENUM;

#define C_CAN_MSK2_MDIR (1<<14)
#define C_CAN_MSK2_MXTD (1<<15)

#define C_CAN_ARB2_DIR (1<<13)
#define C_CAN_ARB2_XTD (1<<14)
#define C_CAN_ARB2_MSGVAL (1<<15)

#define C_CAN_MCTRL_DLC_MASK (0xF)
#define C_CAN_MCTRL_EOB (1<<7)
#define C_CAN_MCTRL_TXRQST (1<<8)
#define C_CAN_MCTRL_RMTEN (1<<9)
#define C_CAN_MCTRL_RXIE (1<<10)
#define C_CAN_MCTRL_TXIE (1<<11)
#define C_CAN_MCTRL_UMASK (1<<12)
#define C_CAN_MCTRL_INTPND (1<<13)
#define C_CAN_MCTRL_MSGLST (1<<14)
#define C_CAN_MCTRL_NEWDAT (1<<15)

typedef struct _C_CAN_FUNCTION
{
	unsigned long CMDREQ;
	union
	{
		unsigned long CMDMSK;
		C_CAN_CMDMSK_ENUM CMDMSKbits;
	};
	unsigned long MASK1;
	unsigned long MASK2;
	unsigned long ARB1;
	unsigned long ARB2;
	unsigned long MCTRL;
	unsigned long DA1;
	unsigned long DA2;
	unsigned long DB1;
	unsigned long DB2;
	unsigned long Reserved[13];
} C_CAN_FUNCTION;


typedef enum
{
	C_CAN_CNTL_INIT = (1<<0),
	C_CAN_CNTL_IE = (1<<1),
	C_CAN_CNTL_SIE = (1<<2),
	C_CAN_CNTL_EIE = (1<<3),
	C_CAN_CNTL_DAR = (1<<5),
	C_CAN_CNTL_CCE = (1<<6),
	C_CAN_CNTL_TEST = (1<<7),
} C_CAN_CNTL;

typedef enum
{
	C_CAN_ERROR_NONE = 0,
	C_CAN_ERROR_STUFF = 1,
	C_CAN_ERROR_FORM = 2,
	C_CAN_ERROR_ACK = 3,
	C_CAN_ERROR_BIT1 = 4,
	C_CAN_ERROR_BIT0 = 5,
	C_CAN_ERROR_CRC = 6,
	C_CAN_ERROR_UNUSED = 7,
} C_CAN_ERROR_CODE;

struct _C_CAN_CANSTAT
{
	C_CAN_ERROR_CODE LEC:3;
	unsigned TXOK:1;
	unsigned RXOK:1;
	unsigned EPASS:1;
	unsigned EWARN:1;
	unsigned BOFF:1;
};

struct _C_CAN_CANEC
{
	unsigned TEC:8;
	unsigned REC:7;
	unsigned RP:1;
};

typedef struct _C_CAN_MODULE
{
	unsigned long CNTL;
	union
	{
		unsigned long STAT;
		struct _C_CAN_CANSTAT STATbits;
	};
	union
	{
		unsigned long EC;
		struct _C_CAN_CANEC ECbits;
	};
	unsigned long BT;
	unsigned long INT;
	unsigned long TEST;
	unsigned long BRPE;
	unsigned long Reserved1;
	C_CAN_FUNCTION Interface1;
	C_CAN_FUNCTION Interface2;
	unsigned long Reserved2[8];

	unsigned long TXREQ1;
	unsigned long TXREQ2;
	unsigned long Reserved3[6];
	unsigned long ND1;
	unsigned long ND2;
	unsigned long Reserved4[6];
	unsigned long IR1;
	unsigned long IR2;
	unsigned long Reserved5[6];
	unsigned long MSGV1;
	unsigned long MSGV2;
	unsigned long Reserved6[6];
	unsigned long CLKDIV;
} C_CAN_MODULE;

#define C_CAN_MESSAGES 32
#define C_CANBT_F(brp, sjw, tseg1, tseg2) ((brp & 0x3F) | ((sjw & 0x03) << 6) | ((tseg1 & 0x0F) << 8) | ((tseg2 & 0x07) << 12))

#endif // C_CAN_H

