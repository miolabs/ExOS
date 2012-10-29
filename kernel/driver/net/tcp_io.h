#ifndef NET_TCP_IO_H
#define NET_TCP_IO_H

#include "tcp.h"
#include "net_io.h"
#include <kernel/fifo.h>

typedef struct 
{
	NET_IO_ENTRY;
    TCP_STATE State;
   	unsigned short LocalPort;
	unsigned short RemotePort;
	IP_ENDPOINT RemoteEP;

	union
	{
		struct
		{
			unsigned long RcvNext;
			EXOS_IO_BUFFER RcvBuffer;
		
			unsigned long SndBase;
			unsigned long SndAck;
			unsigned long SndNext; 
			EXOS_IO_BUFFER SndBuffer;
			TCP_FLAGS SndFlags;
		};
		struct
		{
			EXOS_FIFO AcceptQueue;
		};
	};

	EXOS_MUTEX Mutex;
	unsigned long ServiceWait;
	unsigned long ServiceTime;
} TCP_IO_ENTRY;

typedef struct
{
	EXOS_NODE Node;
	ETH_ADAPTER *Adapter;
	IP_ENDPOINT RemoteEP;
	unsigned short RemotePort;
	unsigned short LocalPort;
	unsigned long Sequence;
} TCP_INCOMING_CONN;

void __tcp_io_initialize();

void net_tcp_io_create(TCP_IO_ENTRY *io, EXOS_IO_FLAGS flags);

#endif // NET_TCP_IO_H