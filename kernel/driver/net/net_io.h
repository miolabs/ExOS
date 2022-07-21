#ifndef NET_NET_IO_H
#define NET_NET_IO_H

#include "adapter.h"
#include <kernel/io.h>
#include <kernel/fifo.h>

typedef enum
{
	NET_IO_DATAGRAM = 0,
	NET_IO_STREAM,
} NET_IO_TYPE;

typedef struct 
{
	io_entry_t;
	NET_ADAPTER *Adapter;
	NET_IO_TYPE ProtocolType;
	unsigned short BufferSize;
	unsigned short Reserved;
} NET_IO_ENTRY;

typedef struct 
{
	io_driver_t IO;
//	int (*Connect)(NET_IO_ENTRY *socket, void *remote, const EXOS_IO_STREAM_BUFFERS *buffers);
	int (*Bind)(NET_IO_ENTRY *socket, void *local);
	int (*Listen)(NET_IO_ENTRY *socket);
//	int (*Accept)(NET_IO_ENTRY *socket, NET_IO_ENTRY *conn_socket, const EXOS_IO_STREAM_BUFFERS *buffers);
	int (*Receive)(NET_IO_ENTRY *socket, void *buffer, unsigned long length, void *remote);
	int (*Send)(NET_IO_ENTRY *socket, void *buffer, unsigned long length, void *remote);
//	int (*Close)(NET_IO_ENTRY *socket, EXOS_IO_STREAM_BUFFERS *buffers);
} NET_PROTOCOL_DRIVER;

void net_io_create(NET_IO_ENTRY *socket, const NET_PROTOCOL_DRIVER *driver, NET_IO_TYPE protocol/*, EXOS_IO_FLAGS flags*/);
//int net_io_connect(NET_IO_ENTRY *socket, void *remote, const EXOS_IO_STREAM_BUFFERS *buffers);
int net_io_bind(NET_IO_ENTRY *socket, void *local);
int net_io_receive(NET_IO_ENTRY *socket, void *buffer, unsigned long length, void *remote);
int net_io_send(NET_IO_ENTRY *socket, void *buffer, unsigned long length, void *remote);
int net_io_listen(NET_IO_ENTRY *socket);
//int net_io_accept(NET_IO_ENTRY *socket, NET_IO_ENTRY *conn_socket, const EXOS_IO_STREAM_BUFFERS *buffers);
//int net_io_close(NET_IO_ENTRY *socket, EXOS_IO_STREAM_BUFFERS *buffers);

#endif // NET_NET_IO_H



