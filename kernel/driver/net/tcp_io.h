#ifndef NET_TCP_IO_H
#define NET_TCP_IO_H

#include "tcp.h"
#include "net_io.h"
#include <kernel/fifo.h>

typedef enum
{
	TCP_STATE_CLOSED,
	TCP_STATE_LISTEN,
	TCP_STATE_SYN_SENT,
	TCP_STATE_SYN_RECEIVED,
	TCP_STATE_ESTABLISHED,
	TCP_STATE_CLOSE_WAIT,
	TCP_STATE_LAST_ACK,
	TCP_STATE_FIN_WAIT_1,
	TCP_STATE_FIN_WAIT_2,
	TCP_STATE_CLOSING,
	TCP_STATE_TIME_WAIT,
} TCP_STATE;

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

			unsigned long SndSeq;
//			unsigned long SndAck;
//			unsigned long SndNext; 
			EXOS_IO_BUFFER SndBuffer;
			TCP_FLAGS SndFlags;
		};
		struct	// for LISTENING STATE
		{
			EXOS_FIFO AcceptQueue;
		};
	};

	EXOS_MUTEX Mutex;
	unsigned short ServiceWait;
	unsigned short ServiceRetry;
	unsigned long ServiceTime;
	EXOS_EVENT CloseEvent;
} TCP_IO_ENTRY;

typedef struct
{
	EXOS_NODE Node;
	NET_ADAPTER *Adapter;
	IP_ENDPOINT RemoteEP;
	unsigned short RemotePort;
	unsigned short LocalPort;
	unsigned long Sequence;
} TCP_INCOMING_CONN;

void __tcp_io_initialize();

void net_tcp_io_create(TCP_IO_ENTRY *io, EXOS_IO_FLAGS flags);

#endif // NET_TCP_IO_H