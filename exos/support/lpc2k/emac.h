#ifndef LPC2K_EMAC_H
#define LPC2K_EMAC_H

#include <net/support/phy.h>
#include <net/adapter.h>
#include <support/lpc2k/cpu.h>

/*------------- Ethernet Media Access Controller (EMAC) ----------------------*/
typedef struct
{
  __IO unsigned long MAC1;                   /* MAC Registers                      */
  __IO unsigned long MAC2;
  __IO unsigned long IPGT;
  __IO unsigned long IPGR;
  __IO unsigned long CLRT;
  __IO unsigned long MAXF;
  __IO unsigned long SUPP;
  __IO unsigned long TEST;
  __IO unsigned long MCFG;
  __IO unsigned long MCMD;
  __IO unsigned long MADR;
  __O  unsigned long MWTD;
  __I  unsigned long MRDD;
  __I  unsigned long MIND;
       unsigned long RESERVED0[2];
  __IO unsigned long SA0;
  __IO unsigned long SA1;
  __IO unsigned long SA2;
       unsigned long RESERVED1[45];
  __IO unsigned long Command;                /* Control Registers                  */
  __I  unsigned long Status;
  __IO unsigned long RxDescriptor;
  __IO unsigned long RxStatus;
  __IO unsigned long RxDescriptorNumber;
  __I  unsigned long RxProduceIndex;
  __IO unsigned long RxConsumeIndex;
  __IO unsigned long TxDescriptor;
  __IO unsigned long TxStatus;
  __IO unsigned long TxDescriptorNumber;
  __IO unsigned long TxProduceIndex;
  __I  unsigned long TxConsumeIndex;
       unsigned long RESERVED2[10];
  __I  unsigned long TSV0;
  __I  unsigned long TSV1;
  __I  unsigned long RSV;
       unsigned long RESERVED3[3];
  __IO unsigned long FlowControlCounter;
  __I  unsigned long FlowControlStatus;
       unsigned long RESERVED4[34];
  __IO unsigned long RxFilterCtrl;           /* Rx Filter Registers                */
  __IO unsigned long RxFilterWoLStatus;
  __IO unsigned long RxFilterWoLClear;
       unsigned long RESERVED5;
  __IO unsigned long HashFilterL;
  __IO unsigned long HashFilterH;
       unsigned long RESERVED6[882];
  __I  unsigned long IntStatus;              /* Module Control Registers           */
  __IO unsigned long IntEnable;
  __O  unsigned long IntClear;
  __O  unsigned long IntSet;
       unsigned long RESERVED7;
  __IO unsigned long PowerDown;
       unsigned long RESERVED8;
  __IO unsigned long Module_ID;
} LPC_EMAC_TypeDef;

extern LPC_EMAC_TypeDef *LPC_EMAC;

typedef struct __attribute__((__packed__))
{
	unsigned char Byte[6];
} ETH_MAC;
 
#define ETH_MAX_FRAME_LEN	1536	// Max Ethernet Frame Length

// Default values
#define MAC_IPGT_FULL_DUPLEX	0x15  // Recommended value for Full Duplex (eq 96/960ns)
#define MAC_IPGT_HALF_DUPLEX	0x12  // Recommended value for Half Duplex (eq 96/960ns)
#define MAC_IPGR_DEFAULT		0x12  // Recommended value (eq 96/960ns)
#define MAC_CLRT_DEFAULT		0x370F

#define OLD_EMAC_MODULE_ID ((0x3902 << 16) | 0x2000) // value to detect rev '-' and workaround

// MAC_INTxx bits
#define MAC_INT_RX_DONE	0x00000008
#define MAC_INT_TX_DONE	0x00000080

// MAC1 bits
#define MAC1_RECEIVE_ENABLE				(1<<0)
#define MAC1_PASS_ALL					(1<<1)
#define MAC1_RX_FLOW_CONTROL			(1<<2)
#define MAC1_TX_FLOW_CONTROL			(1<<3)
#define MAC1_LOOPBACK					(1<<4)

#define MAC1_RESET_TX					(1<<8)
#define MAC1_RESET_MCS_TX				(1<<9)
#define MAC1_RESET_RX					(1<<10)
#define MAC1_RESET_MCS_RX				(1<<11)

#define MAC1_SIM_RESET					(1<<14)
#define MAC1_SOFT_RESET					(1<<15)

// MAC2 bits
#define MAC2_FULL_DUPLEX				(1<<0)
#define MAC2_FRAME_LEN_CHECK			(1<<1)
#define MAC2_HUGE_FRAME					(1<<2)
#define MAC2_DELAYED_CRC				(1<<3)
#define MAC2_CRC_ENABLE					(1<<4)
#define MAC2_PAD_ENABLE					(1<<5)
#define MAC2_VLAN_PAD_ENABLE			(1<<6)
#define MAC2_AUTO_DETECT_PAD			(1<<7)
#define MAC2_PURE_PREAMBLE_ENFORCE		(1<<8)
#define MAC2_LONG_PREAMBLE_ENFORCE		(1<<9)
#define MAC2_NO_BACKOFF					(1<<12)
#define MAC2_BACK_PRESSURE				(1<<13)
#define MAC2_EXCESS_DEFER				(1<<14)

// MAC_COMMAND bits
#define MAC_COMMAND_RX_ENABLE		(1<<0)
#define MAC_COMMAND_TX_ENABLE		(1<<1)
#define MAC_COMMAND_REG_RESET		(1<<3)
#define MAC_COMMAND_TX_RESET		(1<<4)
#define MAC_COMMAND_RX_RESET		(1<<5)
#define MAC_COMMAND_PASS_RUNT_FRAME	(1<<6)
#define MAC_COMMAND_PASS_RX_FILTER	(1<<7)
#define MAC_COMMAND_TX_FLOW_CONTROL	(1<<8)
#define MAC_COMMAND_RMII			(1<<9)
#define MAC_COMMAND_FULL_DUPLEX		(1<<10)

// MAC_MCFG bits
#define MAC_MCFG_SCAN_INCREMENT		(1<<0)
#define MAC_MCFG_SUPPRESS_PREAMBLE	(1<<1)
#define MAC_MCFG_RESET_MII			(1<<15)

#define MAC_MCFG_CLK_DIV4		(1<<2)
#define MAC_MCFG_CLK_DIV6		(2<<2)
#define MAC_MCFG_CLK_DIV8		(3<<2)
#define MAC_MCFG_CLK_DIV10		(4<<2)
#define MAC_MCFG_CLK_DIV14		(5<<2)
#define MAC_MCFG_CLK_DIV20		(6<<2)
#define MAC_MCFG_CLK_DIV28		(7<<2)

// MAC_SUPP bits
#define MAC_SUPP_SPEED	(1<<8) // Sets RMII in 100MBPS mode

// MAC_RXFILTERCTRL bits
#define MAC_RXFC_UNICAST_EN			(1<<0)
#define MAC_RXFC_BROADCAST_EN		(1<<1)
#define MAC_RXFC_MULTICAST_EN		(1<<2)
#define MAC_RXFC_UNICAST_HASH_EN	(1<<3)
#define MAC_RXFC_MULTICAST_HASH_EN	(1<<4)
#define MAC_RXFC_PERFECT_EN			(1<<5)
#define MAC_RXFC_MAGIC_PKT_WOL		(1<<12)
#define MAC_RXFC_RXFILTER_WOL		(1<<13)

// MII Management Command Register
#define MAC_MCMD_READ	0x00000001  /* MII Read                          */
#define MAC_MCMD_SCAN	0x00000002  /* MII Scan continuously             */

// MII Management Address Register
#define MAC_MADR_REG_ADDR	0x0000001F  /* MII Register Address Mask         */
#define MAC_MADR_PHY_ADDR	0x00001F00  /* PHY Address Mask                  */

// MII Management Indicators Register
#define MAC_MIND_BUSY			0x00000001  /* MII is Busy                       */
#define MAC_MIND_SCAN			0x00000002  /* MII Scanning in Progress          */
#define MAC_MIND_NOT_VAL		0x00000004  /* MII Read Data not valid           */
#define MAC_MIND_MII_LINK_FAIL	0x00000008  /* MII Link Failed                   */


// prototypes
int emac_initialize(ETH_MAC *mac, void (*handler)(void *), void *state);
ETH_LINK emac_init_link();

// emac_mem implemented functions
void *emac_get_input_buffer(unsigned long *psize);
void emac_discard_input(void *data);
void *emac_get_output_buffer(unsigned long size);
int emac_send_output(NET_MBUF *mbuf, NET_CALLBACK callback, void *state);

#endif // LPC_EMAC_H
