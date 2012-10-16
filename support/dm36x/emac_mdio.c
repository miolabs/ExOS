#include "emac_mdio.h"
#include "emac.h"

#define EMAC_MDIO_CLOCK 1000000

static EMAC_MDIO *_mdio = (EMAC_MDIO *)0x01D0B000;

#ifndef EMAC_MDIO_PHY_ID
#define EMAC_MDIO_PHY_ID 1
#endif

static void _write_phy(PHYREG reg, unsigned short value);
static unsigned short _read_phy(PHYREG reg);
static const PHY_HANDLER _phy = { .Read = _read_phy, .Write = _write_phy };

static inline void _mdio_wait();

void emac_mdio_reset(int emac_clk)
{
	unsigned int clk_div = (emac_clk / EMAC_MDIO_CLOCK) - 1;
	
	_mdio->CONTROL = MDIO_CONTROL_ENABLE |
		(clk_div & MDIO_CONTROL_CLKDIV_MASK);

	phy_reset(&_phy);
}

static unsigned short _read_phy(PHYREG reg)
{
	_mdio_wait();
	_mdio->USERACCESS0 = MDIO_USERACCESS_GO |
		       ((reg & 0x1f) << MDIO_USERACCESS_REGADR_BIT) |
		       ((EMAC_MDIO_PHY_ID & 0x1f) << MDIO_USERACCESS_PHYADR_BIT);
	_mdio_wait();
	return _mdio->USERACCESS0 & MDIO_USERACCESS_DATA_MASK;
}

static void _write_phy(PHYREG reg, unsigned short value)
{
	_mdio_wait();
	_mdio->USERACCESS0 = MDIO_USERACCESS_GO |
		   MDIO_USERACCESS_WRITE |
		   ((reg & 0x1f) << MDIO_USERACCESS_REGADR_BIT) |
		   ((EMAC_MDIO_PHY_ID & 0x1f) << MDIO_USERACCESS_PHYADR_BIT) |
		   (value & MDIO_USERACCESS_DATA_MASK);
}

static inline void _mdio_wait()
{
	while ((_mdio->USERACCESS0 & MDIO_USERACCESS_GO) != 0);
}

ETH_LINK emac_check_link()
{
	ETH_LINK mode = phy_link_state(&_phy);
	return mode;
}

