#ifndef PHY_KSZ8001_H
#define PHY_KSZ8001_H

#include <kernel/driver/net/support/phy.h>

typedef enum
{
	PHYR_KSZ_RXERC = 0x15,		// RXER (Rx Error) Counter
	PHYR_KSZ_ICS = 0x1B,		// Interrupt Control/Status Register 
	PHYR_KSZ_LCS = 0x1D,		// Link Control/Status Register
	PHYR_KSZ_PCR = 0x1E,		// PHY Control Register
	PHYR_KSZ_100TPCR = 0x1F,	// 100BASE-TX PHY Control Register
} phy_reg_ksz_t;

typedef enum
{
	PHY_KSZ_100TPCR_PAIRSWAP_DISABLE = (1<<13),
	PHY_KSZ_100TPCR_ENERGY_DETECT = (1<<12),
	PHY_KSZ_100TPCR_FORCE_LINK = (1<<11),
	PHY_KSZ_100TPCR_POWER_SAVING = (1<<10),
} phy_ksz_100tpcr_bits_t;

typedef enum 
{
	PHY_KSZ_OPMODE_NEGOTIATING = 0,
	PHY_KSZ_OPMODE_10M_HALF,
	PHY_KSZ_OPMODE_100M_HALF,
	PHY_KSZ_OPMODE_RESERVED3,
	PHY_KSZ_OPMODE_RESERVED4,
	PHY_KSZ_OPMODE_10M_FULL,
	PHY_KSZ_OPMODE_100M_FULL,
	PHY_KSZ_OPMODE_ISOLATE
} phy_ksz_opmode_t;

extern const phy_handler_t __phy_ksz8001;

#endif // PHY_KSZ8001_H 

