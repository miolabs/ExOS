#ifndef STM32F4_ETH_H
#define STM32F4_ETH_H

#include <kernel/driver/net/adapter.h>

#define RDES0_OWN	(1<<31)
#define RDES0_AFM	(1<<30)
#define RDES0_FL_BIT	16
#define RDES0_FL_MASK	(0x3fff << RDES0_FL_BIT)
#define RDES0_ES	(1<<15)
#define RDES0_DE	(1<<14)
#define RDES0_SAF	(1<<13)
#define RDES0_LE	(1<<12)
#define RDES0_OE	(1<<11)
#define RDES0_VLAN	(1<<10)
#define RDES0_FS	(1<<9)
#define RDES0_LS	(1<<8)
#define RDES0_IPHCE_TSV	(1<<7)
#define RDES0_LCO	(1<<6)
#define RDES0_FT	(1<<5)
#define RDES0_RWT	(1<<4)
#define RDES0_DRE	(1<<2)
#define RDES0_CE	(1<<1)
#define RDES0_PCE_ESA	(1<<0)

#define RDES1_DIC	(1<<31)
#define RDES1_RBS2_BIT	16
#define RDES1_RBS2_MASK (0x1fff << RDES1_RBS2_BIT)
#define RDES1_RER	(1<<15)
#define RDES1_RCH	(1<<14)
#define RDES1_RBS_BIT	0
#define RDES1_RBS_MASK (0x1fff << RDES1_RBS_BIT)

#define RDES4_PV	(1<<13)
#define RDES4_PFT	(1<<12)
#define RDES4_PMT_BIT	8
#define RDES4_PMT_MASK	(0xf << RDES4_PMT_BIT)
#define RDES4_IPV6PR	(1<<7)
#define RDES4_IPV4PR	(1<<6)
#define RDES4_IPCB	(1<<5)
#define RDES4_IPPE	(1<<4)
#define RDES4_IPHE	(1<<3)
#define RDES4_IPPT_BIT	0
#define RDES4_IPPT_MASK	(0x7 << RDES4_IPPT_BIT)

typedef struct { unsigned rdes[8]; } rx_edesc_t;
typedef struct { unsigned tdes[8]; } tx_edesc_t;


extern const net_driver_t __stm32_eth_driver;

#endif // STM32F4_ETH_H


