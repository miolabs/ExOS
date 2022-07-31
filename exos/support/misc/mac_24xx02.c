#include "mac_24xx02.h"
#include <support/misc/eeprom.h>

int mac_24xx02_get(HW_ADDR *mac)
{
	unsigned char buf[8];
	if (eeprom_initialize())
	{
		EEPROM_RESULT res = eeprom_read(buf, 0xF8, 8);
		if (res == EEPROM_RES_OK)
		{
			*mac = (HW_ADDR) { buf[2], buf[3], buf[4], buf[5], buf[6], buf[7] };
			return 1;
		}
	}
	return 0;
}

#if !defined EXOS_NO_NET && !defined MAC24XX02_DISABLE
#include <net/board.h>

void net_board_set_mac_address(NET_ADAPTER *adapter, int index)
{
	HW_ADDR mac;
	if (mac_24xx02_get(&mac))
	{
		adapter->MAC = mac;
	}
}
#endif

