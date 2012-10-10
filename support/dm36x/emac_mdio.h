#ifndef DM36X_EMAC_MDIO_H
#define DM36X_EMAC_MDIO_H

typedef volatile struct
{
	unsigned long VERSION;
	unsigned long CONTROL;
	unsigned long ALIVE;
	unsigned long LINK;
	unsigned long LINKINTRAW;
	unsigned long LINKINTMASKED;
	unsigned long Reserved18;
	unsigned long Reserved1C;
	unsigned long USERINTRAW;
	unsigned long USERINTMASKED;
	unsigned long USERINTSET;
	unsigned long USERINTCLEAR;
	unsigned long Reserved30[20];
	unsigned long USERACCESS0;
	unsigned long USERPHYSEL0;
	unsigned long USERACCESS1;
	unsigned long USERPHYSEL1;
} EMAC_MDIO;

#define MDIO_CONTROL_IDLE (1<<31)
#define MDIO_CONTROL_ENABLE (1<<30)
#define MDIO_CONTROL_CLKDIV_MASK (0xFFFF)

#define MDIO_USERACCESS_GO (1<<31)
#define MDIO_USERACCESS_WRITE (1<<30)
#define MDIO_USERACCESS_ACK (1<<29)
#define MDIO_USERACCESS_REGADR_BIT (21)
#define MDIO_USERACCESS_PHYADR_BIT (16)
#define MDIO_USERACCESS_DATA_MASK (0xFFFF)

//////////////////////////////////////
// PHY Definitions

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

typedef enum
{
	PHY_ID_DP83848 = 0x20005C90,
	PHY_ID_KSZ8001 = 0x00221610,
} PHY_ID;

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



// prototypes
void emac_mdio_reset(int emac_clk);
int emac_mdio_read(int phy_id, int phy_reg);
void emac_mdio_write(int phy_id, int phy_reg, unsigned short phy_data);
int emac_mdio_init_phy(int phy_id);


#endif // DM36X_EMAC_MDIO_H
