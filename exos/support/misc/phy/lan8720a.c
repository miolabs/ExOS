#include "lan8720a.h"

static bool _reset(phy_t *phy);
static bool _read_link_state(phy_t *phy);
const phy_handler_t __phy_lan8720a = (phy_handler_t) {
	.Reset = _reset, /*.RestartNeg = _restart_neg,*/
	.ReadLinkState = _read_link_state };

static bool _reset(phy_t *phy)
{
	return (phy->Id & 0xFFFFFFF0) == 0x0007C0F0;
}

static bool _read_link_state(phy_t *phy)
{
	const phy_driver_t *driver = phy->Driver;
	unsigned short reg = driver->Read(phy->Unit, PHYR_LAN8720A_SP_CSR);
	phy->Link = ((reg & PHY_LAN8720A_S_CSR_FULLDUPLEX) ? ETH_LINK_FULL_DUPLEX : ETH_LINK_HALF_DUPLEX)
		| ((reg & PHY_LAN8720A_S_CSR_100BASET) ? ETH_LINK_100M : ETH_LINK_10M);
	return true;
}


