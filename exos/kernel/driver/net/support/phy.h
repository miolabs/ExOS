#ifndef SUPPORT_NET_PHY_H
#define SUPPORT_NET_PHY_H

#include <stdbool.h>

//#ifndef PHY_BASE_ADDR
//#define PHY_BASE_ADDR	0x0100
//#endif

typedef unsigned long phy_id_t;

typedef enum _phy_regs
{
	PHYR_BCR = 0x00,	// Basic Control Register
	PHYR_BSR,		// Basic Status Register
	PHYR_ID1,		// PHY Identifier I
	PHYR_ID2,		// PHY Identifier II
	PHYR_ANAD,		// Auto-Negotiation Advertisement
	PHYR_ANLPA,		// Auto-Negotiation Link Partner Ability
	PHYR_ANEXP,      // Auto-Negotiation Expansion Register
	PHYR_ANNP,		// Auto-Negotiation Next Page TX
	PHYR_LPNPA,		// Link Partner Next Page Ability
} phy_reg_t;

#define PHY_BCR_RESET			(1<<15)
#define PHY_BCR_LOOPBACK		(1<<14)
#define PHY_BCR_100M			(1<<13)
#define PHY_BCR_AUTO			(1<<12)
#define PHY_BCR_POWERDOWN		(1<<11)
#define PHY_BCR_ISOLATE			(1<<10)
#define PHY_BCR_RESTART_AUTO	(1<<9)
#define PHY_BCR_FULL_DUPLEX		(1<<8)
#define PHY_BCR_COL_TEST		(1<<7)
#define PHY_BCR_ENABLE_TX		(1<<0)

#define PHY_BSR_100M_T4			(1<<15)
#define PHY_BSR_100M_FULL		(1<<14)
#define PHY_BSR_100M_HALF		(1<<13)
#define PHY_BSR_10M_FULL		(1<<12)
#define PHY_BSR_10M_HALF		(1<<11)
#define PHY_BSR_NO_PREAMBLE		(1<<6)
#define PHY_BSR_AUTO_COMPLETE	(1<<5)
#define PHY_BSR_REMOTE_FAULT	(1<<4)
#define PHY_BSR_AUTO_ABILITY	(1<<3)
#define PHY_BSR_LINK_UP			(1<<2)
#define PHY_BSR_JABBER_DETECTED	(1<<1)
#define PHY_BSR_EXTENDED_CAP	(1<<0)


typedef struct 
{
	void (*Write)(unsigned unit, phy_reg_t reg, unsigned short value);
	unsigned short (*Read)(unsigned unit, phy_reg_t reg);
} phy_driver_t;

typedef struct __phy_handler phy_handler_t;

typedef enum 
{
	ETH_LINK_NONE = 0,
	ETH_LINK_AUTO = 1,
	ETH_LINK_10M = 2,
	ETH_LINK_100M = 4,
	ETH_LINK_HALF_DUPLEX = 8,
	ETH_LINK_FULL_DUPLEX = 16,
} eth_link_t;

typedef struct
{
	const phy_driver_t *Driver;
	const phy_handler_t *Handler;
	unsigned char Unit;
	eth_link_t Link;
	phy_id_t Id;
} phy_t;


struct __phy_handler
{
	bool (*Reset)(phy_t *phy);
	bool (*RestartNeg)(phy_t *phy);
	bool (*ReadLinkState)(phy_t *phy);
};

// prototypes
bool phy_create(phy_t *phy, const phy_driver_t *driver, unsigned unit, const phy_handler_t *handler);
bool phy_restart_neg(phy_t *phy);
eth_link_t phy_link_state(phy_t *phy);

#endif // SUPPORT_NET_PHY_H
