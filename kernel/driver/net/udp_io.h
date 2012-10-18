#ifndef NET_UDP_IO_H
#define NET_UDP_IO_H

#include "udp.h"
#include "net_io.h"

typedef struct 
{
	NET_IO_ENTRY;
   	unsigned short LocalPort;
	EXOS_FIFO Incoming;
} UDP_IO_ENTRY;

void __udp_io_initialize();
UDP_IO_ENTRY *__udp_io_find_io(ETH_ADAPTER *adapter, unsigned short port);

void net_udp_create_io(UDP_IO_ENTRY *io, EXOS_IO_FLAGS flags);
int net_udp_bind(UDP_IO_ENTRY *io, IP_PORT_ADDR *local);

#endif // NET_UDP_IO_H

