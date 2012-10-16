#include "apipa.h"
#include <support/net_hal.h>

void net_apipa_set_ip_address(ETH_ADAPTER *adapter)
{
	adapter->IP = (IP_ADDR) { 169, 254, adapter->MAC.Bytes[4], adapter->MAC.Bytes[5] };
	adapter->NetMask = (IP_ADDR) { 255, 255, 0, 0 };
}

void hal_net_set_ip_address(ETH_ADAPTER *adapter, int index)
{
	net_apipa_set_ip_address(adapter);
}

