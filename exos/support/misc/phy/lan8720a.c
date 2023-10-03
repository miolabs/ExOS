#include "lan8720a.h"
#include <kernel/verbose.h>
 
static bool _reset(phy_t *phy);
static bool _read_link_state(phy_t *phy);
const phy_handler_t __phy_lan8720a = (phy_handler_t) {
	.Reset = _reset, /*.RestartNeg = _restart_neg,*/
	.ReadLinkState = _read_link_state };

static bool _reset(phy_t *phy)
{
	const phy_driver_t *driver = phy->Driver;

	bool chk_id = false;
	switch(phy->Id & 0xFFFFFFF0)
	{
		case PHY_ID_LAN8720A:
			verbose(VERBOSE_COMMENT, "lan87xx", "phy is LAN8720A");
			chk_id = true;
			break;
		case PHY_ID_LAN8742A:	
			verbose(VERBOSE_COMMENT, "lan87xx", "phy is LAN8742A");
			chk_id = true;
			break;
	}
	if (chk_id)
	{
		driver->Write(phy->Unit, PHYR_BCR, PHY_BCR_AUTO | PHY_BCR_RESTART_AUTO);
	}
	return chk_id;
}

static bool _read_link_state(phy_t *phy)
{
	const phy_driver_t *driver = phy->Driver;
	unsigned short bsr = driver->Read(phy->Unit, PHYR_BSR);
	if (bsr & PHY_BSR_LINK_UP)
	{
		unsigned short csr = driver->Read(phy->Unit, (phy_reg_t)PHYR_LAN8720A_SP_CSR);
		phy->Link = ((csr & PHY_LAN8720A_S_CSR_FULLDUPLEX) ? ETH_LINK_FULL_DUPLEX : ETH_LINK_HALF_DUPLEX)
			| ((csr & PHY_LAN8720A_S_CSR_100BASET) ? ETH_LINK_100M : ETH_LINK_10M);
	}
	else
	{
		phy->Link = ETH_LINK_NONE;
	}
	return true;
}


