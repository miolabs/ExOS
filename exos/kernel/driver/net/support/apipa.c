#include "apipa.h"
#include <net/board.h>

void net_apipa_set_ip_address(NET_ADAPTER *adapter)
{
	adapter->IP = (IP_ADDR) { 169, 254, adapter->MAC.Bytes[4], adapter->MAC.Bytes[5] };
	adapter->NetMask = (IP_ADDR) { 255, 255, 0, 0 };
}

__weak
void net_board_set_ip_address(NET_ADAPTER *adapter, int index) 
{
	net_apipa_set_ip_address(adapter);
}


