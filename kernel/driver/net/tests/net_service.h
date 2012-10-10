#ifndef NET_USER_H
#define NET_USER_H

#include "net/net.h"

//prototypes
void net_user_initialize();
ETH_DRIVER *net_user_wait_ready();

//weak
ETH_DRIVER *net_user_driver_register() __weak;

#endif // NET_USER_H
