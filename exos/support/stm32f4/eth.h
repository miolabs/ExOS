#ifndef STM32F4_ETH_H
#define STM32F4_ETH_H

#include <kernel/driver/net/adapter.h>

#define RDES0_OWN		(1<<31)
#define RDES0_AFM		(1<<30)
#define RDES0_FL_BIT	16
#define RDES0_FL_MASK	(0x3fff << RDES0_FL_BIT)
#define RDES0_ES		(1<<15)
#define RDES0_DE		(1<<14)
#define RDES0_SAF		(1<<13)
#define RDES0_LE		(1<<12)
#define RDES0_OE		(1<<11)
#define RDES0_VLAN		(1<<10)
#define RDES0_FS		(1<<9)
#define RDES0_LS		(1<<8)
#define RDES0_IPHCE_TSV	(1<<7)
#define RDES0_LCO		(1<<6)
#define RDES0_FT		(1<<5)
#define RDES0_RWT		(1<<4)
#define RDES0_DRE		(1<<2)
#define RDES0_CE		(1<<1)
#define RDES0_PCE_ESA	(1<<0)

#define RDES1_DIC		(1<<31)
#define RDES1_RBS2_BIT	16
#define RDES1_RBS2_MASK (0x1fff << RDES1_RBS2_BIT)
#define RDES1_RER		(1<<15)
#define RDES1_RCH		(1<<14)
#define RDES1_RBS_BIT	0
#define RDES1_RBS_MASK (0x1fff << RDES1_RBS_BIT)

#define RDES4_PV		(1<<13)
#define RDES4_PFT		(1<<12)
#define RDES4_PMT_BIT	8
#define RDES4_PMT_MASK	(0xf << RDES4_PMT_BIT)
#define RDES4_IPV6PR	(1<<7)
#define RDES4_IPV4PR	(1<<6)
#define RDES4_IPCB		(1<<5)
#define RDES4_IPPE		(1<<4)
#define RDES4_IPHE		(1<<3)
#define RDES4_IPPT_BIT	0
#define RDES4_IPPT_MASK	(0x7 << RDES4_IPPT_BIT)

typedef struct { unsigned rdes[8]; } rx_edesc_t;

#define TDES0_OWN	(1<<31)
#define TDES0_IC	(1<<30)		// Interrupt on Completion
#define TDES0_LS	(1<<29)		// Last Segment
#define TDES0_FS	(1<<28)		// First Segment
#define TDES0_DC	(1<<27)		// Disable CRC
#define TDES0_DP	(1<<26)		// Disable Padding
#define TDES0_TTSE	(1<<25)		// Transmit Time Stamp Enable
#define TDES0_CIC_DISABLED				(0<<22)
#define TDES0_CIC_IP_HDR_ONLY			(1<<22)
#define TDES0_CIC_IP_HDR_PAYLOAD_SOFT	(2<<22)
#define TDES0_CIC_IP_HDR_PAYLOAD_FULL	(3<<22)
#define TDES0_TCH	(1<<20)
#define TDES0_TTSS	(1<<17)		// Transmit Time Stamp Status
#define TDES0_IHE	(1<<16)		// IP Header Error
#define TDES0_ES	(1<<15)		// Error Summary
#define TDES0_JT	(1<<14)		// Jabber Timeout
#define TDES0_FF	(1<<13)		// Frame Flushed
#define TDES0_IPE	(1<<12)		// IP Payload Error
#define TDES0_LCA	(1<<11)		// Lost CArrier
#define TDES0_NC	(1<<10)		// No Carrier
#define TDES0_LCO	(1<<9)		// Late COllision
#define TDES0_EC	(1<<8)		// Excessive Collision
#define TDES0_VF	(1<<7)		// VLAN Frame
#define TDES0_CC_BIT	3
#define TDES0_CC_MASK	(0xf << TDES0_CC_BIT)		// Collision Count
#define TDES0_ED	(1<<2)		// Excessive Deferral
#define TDES0_UF	(1<<1)		// UnderFlow error
#define TDES0_DB	(1<<0)		// Deferred Bit

typedef struct { unsigned tdes[8]; } tx_edesc_t;

extern const net_driver_t __stm32_eth_driver;

#endif // STM32F4_ETH_H


