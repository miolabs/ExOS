#include "phy.h"
#include <kernel/types.h>

static PHY_ID _identify(const PHY_HANDLER *phy)
{
	unsigned long identifier = (phy->Read(PHYR_ID1) << 16) | phy->Read(PHYR_ID2);
	return (PHY_ID)(identifier & 0xfffffff0);	// exclude revision number
}

int phy_reset(const PHY_HANDLER *phy)
{
	// reset PHY
	phy->Write(PHYR_BCR, PHY_BCR_RESET);
	int startup_time;
	for(startup_time = 1000000; startup_time > 0; startup_time--)
	{
		unsigned short pv = phy->Read(PHYR_BCR); 
		if (0 == (pv & (PHY_BCR_RESET | PHY_BCR_POWERDOWN))) break;
	}
	if (startup_time == 0) return 0; // no response from PHY

	PHY_ID id = _identify(phy);
	
	// custom (specific) initialization
	if (id == PHY_ID_KSZ8001)
	{
		PHY_KSZ_100TPCR_BITS ctrl = phy->Read(PHYR_KSZ_100TPCR);
		ctrl &= ~PHY_KSZ_100TPCR_PAIRSWAP_DISABLE;
		phy->Write(PHYR_KSZ_100TPCR, ctrl);
	}
	else if (id == PHY_ID_DP83848)
	{
		unsigned short ctrl = phy->Read(PHYR_DP83848_PHYCR);
		ctrl |= 0x8000;	// enable auto-neg auto-mdix capability
		phy->Write(PHYR_DP83848_PHYCR, ctrl);
	}
	else if ((id & 0xFFFFFFF0) == PHY_ID_LAN8720A)
	{
		// TODO: currenty nothing, all ok by default
	}

	return 1;
}

void phy_restart_neg(const PHY_HANDLER *phy)
{
	unsigned short bcr = PHY_BCR_AUTO | PHY_BCR_RESTART_AUTO;
	phy->Write(PHYR_BCR, bcr);
}

ETH_LINK phy_link_state(const PHY_HANDLER *phy)
{
	ETH_LINK mode = ETH_LINK_NONE;
	unsigned short bsr = phy->Read(PHYR_BSR); 
	if (0 != (bsr & PHY_BSR_AUTO_COMPLETE))
	{
		unsigned short reg;
		PHY_ID id = _identify(phy);
		if (id == PHY_ID_KSZ8001)
		{
			reg = phy->Read(PHYR_KSZ_100TPCR);

			switch((PHY_KSZ_OPMODE)((reg >> 2) & 0x7)) // Operation Mode
			{
				case PHY_KSZ_OPMODE_10M_HALF:	mode = ETH_LINK_10M | ETH_LINK_HALF_DUPLEX;		break;
				case PHY_KSZ_OPMODE_100M_HALF:	mode = ETH_LINK_100M | ETH_LINK_HALF_DUPLEX;	break;
				case PHY_KSZ_OPMODE_10M_FULL:	mode = ETH_LINK_10M | ETH_LINK_FULL_DUPLEX;		break;
				case PHY_KSZ_OPMODE_100M_FULL:	mode = ETH_LINK_100M | ETH_LINK_FULL_DUPLEX;	break;
			}
		}
		else if (id == PHY_ID_DP83848)
		{
			reg = phy->Read(PHYR_DP83848_PHYSTA);
			mode = ((reg & PHY_DP83848_PHYSTA_SPEED) ? ETH_LINK_10M : ETH_LINK_100M)
				| ((reg & PHY_DP83848_PHYSTA_DUPLEX) ? ETH_LINK_FULL_DUPLEX : ETH_LINK_HALF_DUPLEX);
		}
		else if ((id & 0xFFFFFFF0) == PHY_ID_LAN8720A)
		{
			reg = phy->Read(PHYR_LAN8720A_SP_CSR);
			mode = ((reg & PHY_LAN8720A_S_CSR_FULLDUPLEX) ? ETH_LINK_FULL_DUPLEX : ETH_LINK_HALF_DUPLEX)
				| ((reg & PHY_LAN8720A_S_CSR_100BASET) ? ETH_LINK_100M : ETH_LINK_10M);
			// NOTE: maybe this is std and may me merged (it is functionally equivalent to KSZ8001 for the case)
		}
	}
	return mode;
}


