// IP Stack initialization
// by Miguel Fides

#include "net.h"
#include "arp_tables.h"
#include "adapter.h"
#include "ip.h"
#include "board.h"

void net_initialize()
{
	net_arp_tables_initialize();
	net_ip_initialize();
	net_adapter_initialize();
}

// compare hw addresses
int net_equal_hw_addr(HW_ADDR *a, HW_ADDR *b)
{
	for(int i = 0; i < sizeof(HW_ADDR); i++)
	{
		if (a->Bytes[i] != b->Bytes[i]) return 0;
	}
	return 1;
}

__weak NET_ADAPTER *net_board_get_adapter(int index)
{
	return NULL;
}

static const HW_ADDR _dummy_mac = { 1, 2, 3, 4, 5, 6 };

__weak void net_board_set_mac_address(NET_ADAPTER *adapter, int index)
{
	adapter->MAC = _dummy_mac;
	adapter->MAC.Bytes[5] += index;
}






