#ifndef NET_BOARD_H
#define NET_BOARD_H

#include <net/adapter.h>

net_adapter_t *net_board_get_adapter(int index);
void net_board_set_mac_address(net_adapter_t *adapter, int index);
void net_board_set_ip_address(net_adapter_t *adapter, int index);

#endif // NET_BOARD_H
