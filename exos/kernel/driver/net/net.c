// IP Stack initialization
// by Miguel Fides

#include "net.h"
#include "arp_tables.h"
#include "adapter.h"
#include "ip.h"
#include <support/services/init.h>
#include <kernel/panic.h>

static void _register();
EXOS_INITIALIZER(_init, EXOS_INIT_IO_DRIVER, _register);

static mutex_t _mutex;
static list_t _protocol_handlers;

static void _register()
{
	net_adapter_initialize();
	net_arp_tables_initialize();
	net_ip_initialize();
}

// compare hw addresses
bool net_equal_hw_addr(hw_addr_t *a, hw_addr_t *b)
{
	ASSERT(a != NULL && b != NULL, KERNEL_ERROR_NULL_POINTER);

	for(int i = 0; i < sizeof(hw_addr_t); i++)
	{
		if (a->Bytes[i] != b->Bytes[i]) return false;
	}
	return true;
}

static bool _parse_hex(unsigned char *pout, const char c)
{
	if (c >= 'A' && c <= 'F')
	{
		*pout = 10 + (c - 'A');
	}
	else if  (c >= '0' && c <= '9')
	{
		*pout = c - '0';
	}
	else return false;
	return true;
}

bool net_hw_addr_parse(hw_addr_t *addr, const char *mac)
{
	ASSERT(addr != NULL && mac != NULL, KERNEL_ERROR_NULL_POINTER);
	unsigned i = 0;
	while(1)
	{
		unsigned char digit1, digit2;
		if (_parse_hex(&digit1, mac[0]) && _parse_hex(&digit2, mac[1]))
		{
			addr->Bytes[i++] = (digit1 << 4) + digit2;
			if (i == 6)
				return true;
			mac += 2;
			if (mac[0] == ':') mac++;
		}
		else break;
	}
	return false;
}

/*
static const HW_ADDR _dummy_mac = { 1, 2, 3, 4, 5, 6 };

__weak void net_board_set_mac_address(NET_ADAPTER *adapter, int index)
{
	adapter->MAC = _dummy_mac;
	adapter->MAC.Bytes[5] += index;
}
*/





