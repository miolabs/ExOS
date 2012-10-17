#ifndef NET_BOARD_H
#define NET_BOARD_H

#include <net/adapter.h>

ETH_ADAPTER *net_board_get_adapter(int index);
void net_board_set_mac_address(ETH_ADAPTER *adapter, int index);
void net_board_set_ip_address(ETH_ADAPTER *adapter, int index);

#endif // NET_BOARD_H
