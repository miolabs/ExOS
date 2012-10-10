#ifndef HAL_NET_H
#define HAL_NET_H

#include <net/adapter.h>

ETH_ADAPTER *hal_net_get_adapter(int index);
void hal_net_set_mac_address(ETH_ADAPTER *adapter, int index);
void hal_net_set_ip_address(ETH_ADAPTER *adapter, int index);

#endif // HAL_NET_H
