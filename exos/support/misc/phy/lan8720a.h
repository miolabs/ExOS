#ifndef PHY_LAN8720A_H
#define PHY_LAN8720A_H

#include <kernel/driver/net/support/phy.h>

// SMSC LAN8720A PHY
typedef enum
{
	PHYR_LAN8720A_SP_CSR = 0x1F,
} phy_reg_lan8720a_t;

#define PHY_LAN8720A_S_CSR_FULLDUPLEX (1<<4)
#define PHY_LAN8720A_S_CSR_100BASET (1<<3)
#define PHY_LAN8720A_S_CSR_10BASET (1<<2)


extern const phy_handler_t __phy_lan8720a;

#endif // PHY_LAN8720A_H 

