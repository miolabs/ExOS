// IP Stack initialization
// by Miguel Fides

#include "net.h"
#include "arp_tables.h"
#include "adapter.h"
#include "ip.h"

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




