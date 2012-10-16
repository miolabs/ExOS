#ifndef DM36X_EMAC_H
#define DM36X_EMAC_H

#include "intc.h"
#include <net/mbuf.h>
#include <net/support/phy.h>

typedef volatile struct
{
	unsigned long TXIDVER;
	unsigned long TXCONTROL;
	unsigned long TXTEARDOWN;
	unsigned long Reserved00C;
	unsigned long RXIDVER;
	unsigned long RXCONTROL;
	unsigned long RXTEARDOWN;
	unsigned long Reserved01C;
	unsigned long Reserved020[24];
	// +0x080
	unsigned long TXINTSTATRAW;
	unsigned long TXINTSTATMASKED;
	unsigned long TXINTMASKSET;
	unsigned long TXINTMASKCLEAR;
	unsigned long MACINVECTOR;
	unsigned long MACEOIVECTOR;
	unsigned long Reserved098;
	unsigned long Reserved09C;
	unsigned long RXINTSTATRAW;
	unsigned long RXINTSTATMASKED;
	unsigned long RXINTMASKSET;
	unsigned long RXINTMASKCLEAR;
	unsigned long MACINTSTATRAW;
	unsigned long MACINTSTATMASKED;
	unsigned long MACINTMASKSET;
	unsigned long MACINTMASKCLEAR;
	unsigned long Reserved0C0[16];
	// +0x100
	unsigned long RXMBPENABLE;
	unsigned long RXUNICASTSET;
	unsigned long RXUNICASTCLEAR;
	unsigned long RXMAXLEN;
	unsigned long RXBUFFEROFFSET;
	unsigned long RXFILTERLOWTHRESH;
	unsigned long Reserved118;
	unsigned long Reserved11C;
	unsigned long RXFLOWTHRESH[8];
	unsigned long RXFREEBUFFER[8];
	unsigned long MACCONTROL;
	unsigned long MACSTATUS;
	unsigned long EMCONTROL;
	unsigned long FIFOCONTROL;
	unsigned long MACCONFIG;
	unsigned long SOFTRESET;
	unsigned long Reserved178;
	unsigned long Reserved17C;
	unsigned long Reserved180[20];
	// +0x1D0
	unsigned long MACSRCADDRLO;
	unsigned long MACSRCADDRHI;
	unsigned long MACHASH1;
	unsigned long MACHASH2;
	unsigned long BOFFTEST;
	unsigned long TPACETEST;
	unsigned long RXPAUSE;
	unsigned long TXPAUSE;
	unsigned long Reserved1F0[196];
	// +0x500
	unsigned long MACADDRLO;
	unsigned long MACADDRHI;
	unsigned long MACINDEX;
	unsigned long Reserved50C[1];
} EMAC_MODULE;

#define EMAC_MACINTMASK_HOST (1<<1)
#define EMAC_MACINTMASK_STAT (1<<0)

#define EMAC_RXMBP_RXBROADEN (1<<13)
#define EMAC_RXMBP_RXBROADCH_BIT (8)
#define EMAC_RXMBP_RXMULTEN (1<<5)
#define EMAC_RXMBP_RXMULTCH_BIT (0)

#define EMAC_MACCONTROL_TXPTYPE (1<<9)
#define EMAC_MACCONTROL_MIIEN (1<<5)
#define EMAC_MACCONTROL_FULLDUPLEX (1<<0)

#define EMAC_MACADDRLO_CHANNEL_BIT 16
#define EMAC_MACADDRLO_MATCHFILT (1<<19)
#define EMAC_MACADDRLO_VALID (1<<20)

#define EMAC_EOI_RXTHRESH 0
#define EMAC_EOI_RXPULSE 1
#define EMAC_EOI_TXPULSE 2
#define EMAC_EOI_MISC 3

// This struct is at offset 0x600 in EMAC_MODULE
typedef volatile struct
{
	void *TXHDP[8];
	void *RXHDP[8];
	void *TXCP[8];
	void *RXCP[8];
} EMAC_MODULE_POINTERS;

typedef volatile struct
{
	unsigned long CMIDVER;
	unsigned long CMSOFTRESET;
	unsigned long CMEMCONTROL;
	unsigned long CMINTCTRL;
	unsigned long CMTHRESHINTEN;
	unsigned long CMRXINTEN;
	unsigned long CMTXINTEN;
	unsigned long CMMISCINTEN;
	unsigned long Reserved20[8];
	unsigned long CMTHRESHINTSTAT;
	unsigned long CMRXINTSTAT;
	unsigned long CMTXINTSTAT;
	unsigned long CMMISCINTSTAT;
	unsigned long Reserved50[8];
	unsigned long CMRXINTMAX;
	unsigned long CMTXINTMAX;
} EMAC_CONTROL_MODULE;

typedef struct
{
	unsigned PacketLength:16;
	unsigned :10;
	unsigned PASSCRC:1;
	unsigned TDOWNCMPLT:1;
	unsigned EOQ:1;
	unsigned OWNER:1;
	unsigned EOP:1;
	unsigned SOP:1;
} EMAC_PKT_FlagLen;

typedef struct _EMAC_Desc 
{
	struct _EMAC_Desc *Next; /* Pointer to next descriptor in chain */
	unsigned char *Buffer; /* Pointer to data buffer */
	unsigned long BufOffLen;
	union
	{
		EMAC_PKT_FlagLen PktFlgLenBits;
		unsigned long PktFlgLen;
	};
} EMAC_Desc;

#define EMAC_MAX_ETHERNET_PKT_SIZE (1500 + 14 + 4 + 4)

typedef struct __attribute__((__aligned__(4)))
{
	unsigned char Bytes[EMAC_MAX_ETHERNET_PKT_SIZE];
} EMAC_PKT_BUFFER;

/* Packet Flags */
#define EMAC_DSC_FLAG_SOP 0x80000000u
#define EMAC_DSC_FLAG_EOP 0x40000000u
#define EMAC_DSC_FLAG_OWNER 0x20000000u
#define EMAC_DSC_FLAG_EOQ 0x10000000u
#define EMAC_DSC_FLAG_TDOWNCMPLT 0x08000000u
#define EMAC_DSC_FLAG_PASSCRC 0x04000000u
/* Packet Flags, RX */
#define EMAC_DSC_FLAG_JABBER 0x02000000u
#define EMAC_DSC_FLAG_OVERSIZE 0x01000000u
#define EMAC_DSC_FLAG_FRAGMENT 0x00800000u
#define EMAC_DSC_FLAG_UNDERSIZED 0x00400000u
#define EMAC_DSC_FLAG_CONTROL 0x00200000u
#define EMAC_DSC_FLAG_OVERRUN 0x00100000u
#define EMAC_DSC_FLAG_CODEERROR 0x00080000u
#define EMAC_DSC_FLAG_ALIGNERROR 0x00040000u
#define EMAC_DSC_FLAG_CRCERROR 0x00020000u
#define EMAC_DSC_FLAG_NOMATCH 0x00010000u


//prototypes
void emac_initialize(unsigned char *mac_addr);
ETH_LINK emac_check_link();

void *emac_get_input_buffer(unsigned long *psize);
void emac_discard_input_buffer(void *buffer);
void *emac_get_output_buffer(unsigned long size);
int emac_send_output_buffer(NET_MBUF *mbuf, ETH_CALLBACK callback, void *state);

void emac_dm36x_rx_handler() __attribute__((__weak__));

#endif // DM36X_EMAC_H
