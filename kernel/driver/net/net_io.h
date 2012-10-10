#ifndef NET_NET_IO_H
#define NET_NET_IO_H

#include "adapter.h"
#include <kernel/io.h>
#include <kernel/fifo.h>

typedef struct 
{
	EXOS_IO_ENTRY;
	ETH_ADAPTER *Adapter;
	unsigned short LocalPort;
	EXOS_FIFO Incoming;
	// TODO
} NET_IO_ENTRY;

typedef struct 
{
	EXOS_IO_DRIVER IO;
	int (*Bind)(NET_IO_ENTRY *io, void *addr);
	int (*Receive)(NET_IO_ENTRY *io, void *buffer, unsigned long length, void *addr);
	int (*Send)(NET_IO_ENTRY *io, void *buffer, unsigned long length, void *addr);
} NET_PROTOCOL_DRIVER;

typedef struct
{
	IP_ADDR Address;
	unsigned short Port;
} IP_PORT_ADDR;

int net_io_bind(NET_IO_ENTRY *io, IP_PORT_ADDR *ipp);
int net_io_receive(NET_IO_ENTRY *io, void *buffer, unsigned long length, IP_PORT_ADDR *ipp);
int net_io_send(NET_IO_ENTRY *io, void *buffer, unsigned long length, IP_PORT_ADDR *ipp);

// defined in udp.c
void net_udp_create_io(NET_IO_ENTRY *io, EXOS_IO_FLAGS flags);

#endif // NET_NET_IO_H



