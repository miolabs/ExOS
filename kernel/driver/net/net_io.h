#ifndef NET_NET_IO_H
#define NET_NET_IO_H

#include "adapter.h"
#include <kernel/io.h>
#include <kernel/fifo.h>

typedef struct 
{
	EXOS_IO_ENTRY;
	ETH_ADAPTER *Adapter;
	unsigned long Protocol;
} NET_IO_ENTRY;

typedef struct 
{
	EXOS_IO_DRIVER IO;
	int (*Bind)(NET_IO_ENTRY *io, void *local);
	int (*Receive)(NET_IO_ENTRY *io, void *buffer, unsigned long length, void *remote);
	int (*Send)(NET_IO_ENTRY *io, void *buffer, unsigned long length, void *remote);
} NET_PROTOCOL_DRIVER;

int net_io_bind(NET_IO_ENTRY *io, void *local);
int net_io_receive(NET_IO_ENTRY *io, void *buffer, unsigned long length, void *remote);
int net_io_send(NET_IO_ENTRY *io, void *buffer, unsigned long length, void *remote);

#endif // NET_NET_IO_H



