#include "phy.h"

int phy_reset(const PHY_HANDLER *phy, PHY_ID *pid)
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

	// identify PHY
	PHY_ID id = (phy->Read(PHYR_ID1) << 16) | phy->Read(PHYR_ID2);

	// custom (specific) initialization
	if (id == PHY_ID_KSZ8001 || id == PHY_ID_KSZ8721)
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
	*pid = id;
}

void phy_restart_neg(const PHY_HANDLER *phy)
{
	unsigned short bcr = PHY_BCR_AUTO | PHY_BCR_RESTART_AUTO;
	phy->Write(PHYR_BCR, bcr);
}

ETH_LINK phy_link_state(const PHY_HANDLER *phy, PHY_ID id)
{
	ETH_LINK mode = ETH_LINK_NONE;
	unsigned short bsr = phy->Read(PHYR_BSR); 
	if (0 != (bsr & PHY_BSR_AUTO_COMPLETE))
	{
		if (id == PHY_ID_KSZ8001 || id == PHY_ID_KSZ8721)
		{
			unsigned short phycon = phy->Read(PHYR_KSZ_100TPCR);
	
			switch((PHY_KSZ_OPMODE)((phycon >> 2) & 0x7)) // Operation Mode
			{
				case PHY_KSZ_OPMODE_10M_HALF:	mode = ETH_LINK_10M | ETH_LINK_HALF_DUPLEX;		break;
				case PHY_KSZ_OPMODE_100M_HALF:	mode = ETH_LINK_100M | ETH_LINK_HALF_DUPLEX;	break;
				case PHY_KSZ_OPMODE_10M_FULL:	mode = ETH_LINK_10M | ETH_LINK_FULL_DUPLEX;		break;
				case PHY_KSZ_OPMODE_100M_FULL:	mode = ETH_LINK_100M | ETH_LINK_FULL_DUPLEX;	break;
			}
		}
		else if (id == PHY_ID_DP83848)
		{
			unsigned short sta = phy->Read(PHYR_DP83848_PHYSTA);
			mode = ((sta & PHY_DP83848_PHYSTA_SPEED) ? ETH_LINK_10M : ETH_LINK_100M)
			 | ((sta & PHY_DP83848_PHYSTA_DUPLEX) ? ETH_LINK_FULL_DUPLEX : ETH_LINK_HALF_DUPLEX);
		}
	}
	return mode;
}


