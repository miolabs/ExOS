#include "phy.h"
#include <kernel/panic.h>

static phy_id_t _identify(const phy_driver_t *phy, unsigned unit)
{
	unsigned long identifier = (phy->Read(unit, PHYR_ID1) << 16) | phy->Read(unit, PHYR_ID2);
	return (phy_id_t)(identifier & 0xfffffff0);	// NOTE: exclude revision number
}

bool phy_create(phy_t *phy, const phy_driver_t *driver, unsigned unit, const phy_handler_t *handler)
{
	ASSERT(phy != NULL && driver != NULL && handler != NULL, KERNEL_ERROR_NULL_POINTER);
	phy->Driver = driver;
	phy->Handler = handler;
	phy->Unit = unit;

	// reset PHY
	driver->Write(unit, PHYR_BCR, PHY_BCR_RESET);

	int startup_time = 1000000; 
	while(startup_time > 0)
	{
		unsigned short pv = driver->Read(unit, PHYR_BCR); 
		if (0 == (pv & (PHY_BCR_RESET | PHY_BCR_POWERDOWN))) break;
		startup_time--;
	}
	if (startup_time == 0) return false; // no response from PHY

	phy->Id = _identify(driver, unit);
	
	// TODO: make a driver for DP83848
	if (phy->Id == 0x20005C90) // PHY_ID_DP83848
	{
		unsigned short ctrl = driver->Read(phy->Unit, 0x19);	// PHYCR
		ctrl |= 0x8000;	// enable auto-neg auto-mdix capability
		driver->Write(phy->Unit, 0x19, ctrl);
	}

	return (handler->Reset != NULL) ? handler->Reset(phy) : true;
}

bool phy_restart_neg(phy_t *phy)
{
	ASSERT(phy != NULL, KERNEL_ERROR_NULL_POINTER);
	const phy_handler_t *handler = phy->Handler;
	ASSERT(handler != NULL, KERNEL_ERROR_NULL_POINTER);
	if (handler->RestartNeg)
	{
		return handler->RestartNeg(phy);
	}
	else
	{
		const phy_driver_t *driver = phy->Driver;
		ASSERT(driver != NULL, KERNEL_ERROR_NULL_POINTER);

		unsigned short bcr = PHY_BCR_AUTO | PHY_BCR_RESTART_AUTO;
		driver->Write(phy->Unit, PHYR_BCR, bcr);
		return true;
	}
}

bool phy_read_link_state(phy_t *phy)
{
	ASSERT(phy != NULL, KERNEL_ERROR_NULL_POINTER);
	const phy_handler_t *handler = phy->Handler;
	ASSERT(handler != NULL && handler->RestartNeg, KERNEL_ERROR_NULL_POINTER);
	return handler->ReadLinkState(phy);
}

