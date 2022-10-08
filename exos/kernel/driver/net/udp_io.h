#ifndef NET_UDP_IO_H
#define NET_UDP_IO_H

#include "udp.h"
#include "net_io.h"

typedef struct 
{
	NET_IO_ENTRY;
   	unsigned short LocalPort;
	fifo_t Incoming;
} UDP_IO_ENTRY;

void __udp_io_initialize();
UDP_IO_ENTRY *__udp_io_find_io(net_adapter_t *adapter, unsigned short port);

void net_udp_io_create(UDP_IO_ENTRY *io/*, EXOS_IO_FLAGS flags*/);

#endif // NET_UDP_IO_H

