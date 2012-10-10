#ifndef SUPPORT_NET_PHY_H
#define SUPPORT_NET_PHY_H

// common PHY definitions
#ifndef PHY_BASE_ADDR
#define PHY_BASE_ADDR	0x0100
#endif

typedef enum
{
	PHY_ID_NONE = 0,
	PHY_ID_DP83848 = 0x20005C90,
	PHY_ID_KSZ8001 = 0x00221610,
	PHY_ID_KSZ8721 = 0x00221619,
} PHY_ID;

typedef enum _PHY_REG
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
} PHYREG;

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


// MICREL KSZxxxx PHY
typedef enum
{
	PHYR_KSZ_RXERC = 0x15,		// RXER (Rx Error) Counter
	PHYR_KSZ_ICS = 0x1B,		// Interrupt Control/Status Register 
	PHYR_KSZ_LCS = 0x1D,		// Link Control/Status Register
	PHYR_KSZ_PCR = 0x1E,		// PHY Control Register
	PHYR_KSZ_100TPCR = 0x1F,	// 100BASE-TX PHY Control Register
} PHYREG_KSZ;

typedef enum
{
	PHY_KSZ_100TPCR_PAIRSWAP_DISABLE = (1<<13),
	PHY_KSZ_100TPCR_ENERGY_DETECT = (1<<12),
	PHY_KSZ_100TPCR_FORCE_LINK = (1<<11),
	PHY_KSZ_100TPCR_POWER_SAVING = (1<<10),
} PHY_KSZ_100TPCR_BITS;

typedef enum _PHY_OP_MODE
{
	PHY_KSZ_OPMODE_NEGOTIATING = 0,
	PHY_KSZ_OPMODE_10M_HALF,
	PHY_KSZ_OPMODE_100M_HALF,
	PHY_KSZ_OPMODE_RESERVED3,
	PHY_KSZ_OPMODE_RESERVED4,
	PHY_KSZ_OPMODE_10M_FULL,
	PHY_KSZ_OPMODE_100M_FULL,
	PHY_KSZ_OPMODE_ISOLATE
} PHY_KSZ_OPMODE;

// NATIONAL DP83848x PHY
typedef enum
{
	PHYR_DP83848_PHYSTA = 0x10, // PHY Status Register
	PHYR_DP83848_PHYCR = 0x19, // PHY Control Register
} PHYREG_DP83848;

#define PHY_DP83848_PHYSTA_LINK		(1<<0)
#define PHY_DP83848_PHYSTA_SPEED	(1<<1)
#define PHY_DP83848_PHYSTA_DUPLEX	(1<<2)

typedef struct 
{
	void (*Write)(PHYREG reg, unsigned short value);
	unsigned short (*Read)(PHYREG reg);
} PHY_HANDLER;

typedef enum _ETH_LINK
{
	ETH_LINK_NONE = 0,
	ETH_LINK_AUTO = 1,
	ETH_LINK_10M = 2,
	ETH_LINK_100M = 4,
	ETH_LINK_HALF_DUPLEX = 8,
	ETH_LINK_FULL_DUPLEX = 16,
} ETH_LINK;


// prototypes
int phy_reset(const PHY_HANDLER *phy, PHY_ID *pid);
void phy_restart_neg(const PHY_HANDLER *phy);
ETH_LINK phy_link_state(const PHY_HANDLER *phy, PHY_ID id);

#endif // SUPPORT_NET_PHY_H
