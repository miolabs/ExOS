#include "emac_mdio.h"
#include "emac.h"
#include <kernel/user.h>

#define EMAC_MDIO_CLOCK 1000000

static EMAC_MDIO *_mdio = (EMAC_MDIO *)0x01D0B000;

static inline void _mdio_wait();

void emac_mdio_reset(int emac_clk)
{
	unsigned int clk_div = (emac_clk / EMAC_MDIO_CLOCK) - 1;
	
	_mdio->CONTROL = MDIO_CONTROL_ENABLE |
		(clk_div & MDIO_CONTROL_CLKDIV_MASK);
}

int emac_mdio_read(int phy_id, int phy_reg)
{
	_mdio_wait();
	_mdio->USERACCESS0 = MDIO_USERACCESS_GO |
		       ((phy_reg & 0x1f) << MDIO_USERACCESS_REGADR_BIT) |
		       ((phy_id & 0x1f) << MDIO_USERACCESS_PHYADR_BIT);
	_mdio_wait();
	return _mdio->USERACCESS0 & MDIO_USERACCESS_DATA_MASK;
}

void emac_mdio_write(int phy_id, int phy_reg, unsigned short phy_data)
{
	_mdio_wait();
	_mdio->USERACCESS0 = MDIO_USERACCESS_GO |
		   MDIO_USERACCESS_WRITE |
		   ((phy_reg & 0x1f) << MDIO_USERACCESS_REGADR_BIT) |
		   ((phy_id & 0x1f) << MDIO_USERACCESS_PHYADR_BIT) |
		   (phy_data & MDIO_USERACCESS_DATA_MASK);
}

static inline void _mdio_wait()
{
	while ((_mdio->USERACCESS0 & MDIO_USERACCESS_GO) != 0);
}

int emac_mdio_init_phy(int phy_id)
{
	// reset PHY
	emac_mdio_write(phy_id, PHYR_BCR, PHY_BCR_RESET);
	int startup_time;
	for(startup_time = 100; startup_time > 0; startup_time--)
	{
		unsigned short pv = emac_mdio_read(phy_id, PHYR_BCR); 
		if (0 == (pv & (PHY_BCR_RESET | PHY_BCR_POWERDOWN))) break;
		sleep(1);
	}
	if (startup_time == 0) return 0; // no response from PHY

	// identify PHY
	PHY_ID id = (emac_mdio_read(phy_id, PHYR_ID1) << 16) |
		(emac_mdio_read(phy_id, PHYR_ID2) & 0xFFF0);

	// custom (specific) initialization
	if (id == PHY_ID_KSZ8001)
	{
		PHY_KSZ_100TPCR_BITS ctrl = emac_mdio_read(phy_id, PHYR_KSZ_100TPCR);
		ctrl &= ~PHY_KSZ_100TPCR_PAIRSWAP_DISABLE;
		emac_mdio_write(phy_id, PHYR_KSZ_100TPCR, ctrl);
	}
	else if (id == PHY_ID_DP83848)
	{
		unsigned short ctrl = emac_mdio_read(phy_id, PHYR_DP83848_PHYCR);
		ctrl |= 0x8000;	// enable auto-neg auto-mdix capability
		emac_mdio_write(phy_id, PHYR_DP83848_PHYCR, ctrl);
	}

	unsigned short sta = emac_mdio_read(phy_id, PHYR_BSR);

/*
	// re-negotiate
	unsigned short bcr = PHY_BCR_AUTO | PHY_BCR_RESTART_AUTO;
	emac_mdio_write(phy_id, PHYR_BCR, bcr);
	int negotime;
	unsigned short bsr;
	for(negotime = 5000; negotime > 0; negotime--)
	{
		bsr = emac_mdio_read(phy_id, PHYR_BSR);
		if (0 != (bsr & PHY_BSR_AUTO_COMPLETE)) break;
		sleep(1);
	}
	if (negotime == 0) return 0; // no link negotiated	
*/
	return 1;
}


