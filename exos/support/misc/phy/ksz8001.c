#include "ksz8001.h"

static bool _reset(phy_t *phy);
static bool _read_link_state(phy_t *phy);
const phy_handler_t __phy_ksz8001 = (phy_handler_t) {
	.Reset = _reset, /*.RestartNeg = _restart_neg,*/
	.ReadLinkState = _read_link_state };

static bool _reset(phy_t *phy)
{
	if (phy->Id == 0x00221610)
	{
		const phy_driver_t *driver = phy->Driver;
		phy_ksz_100tpcr_bits_t ctrl = driver->Read(phy->Unit, PHYR_KSZ_100TPCR);
		ctrl &= ~PHY_KSZ_100TPCR_PAIRSWAP_DISABLE;
		driver->Write(phy->Unit, PHYR_KSZ_100TPCR, ctrl);
		return true;
	}
	else return false;
}

static bool _read_link_state(phy_t *phy)
{
	const phy_driver_t *driver = phy->Driver;
	unsigned short reg = driver->Read(phy->Unit, PHYR_KSZ_100TPCR);
	switch((phy_ksz_opmode_t)((reg >> 2) & 0x7)) // Operation Mode
	{
		case PHY_KSZ_OPMODE_10M_HALF:	phy->Link = ETH_LINK_10M | ETH_LINK_HALF_DUPLEX;	break;
		case PHY_KSZ_OPMODE_100M_HALF:	phy->Link = ETH_LINK_100M | ETH_LINK_HALF_DUPLEX;	break;
		case PHY_KSZ_OPMODE_10M_FULL:	phy->Link = ETH_LINK_10M | ETH_LINK_FULL_DUPLEX;	break;
		case PHY_KSZ_OPMODE_100M_FULL:	phy->Link = ETH_LINK_100M | ETH_LINK_FULL_DUPLEX;	break;
	}
	return true;
}


