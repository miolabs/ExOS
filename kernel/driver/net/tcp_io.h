#ifndef NET_TCP_IO_H
#define NET_TCP_IO_H

#include "tcp.h"
#include "net_io.h"

typedef struct 
{
	NET_IO_ENTRY;
    TCP_STATE State;
   	unsigned short LocalPort;
	unsigned short RemotePort;
	IP_ENDPOINT RemoteEP;

	unsigned long RcvNext;
	EXOS_IO_BUFFER RcvBuffer;

	unsigned long SndBase;
	unsigned long SndAck;
	unsigned long SndNext; 
	TCP_FLAGS SndFlags;
	EXOS_IO_BUFFER SndBuffer;

	EXOS_MUTEX Mutex;
	unsigned long ServiceWait;
	unsigned long ServiceTime;
} TCP_IO_ENTRY;


void __tcp_io_initialize();

void net_tcp_io_create(TCP_IO_ENTRY *io, EXOS_IO_FLAGS flags, void *rcv_buffer, unsigned short rcv_length, void *snd_buffer, unsigned short snd_length);
int net_tcp_listen(TCP_IO_ENTRY *io);
int net_tcp_accept(TCP_IO_ENTRY *io);
int net_tcp_connect(TCP_IO_ENTRY *io, IP_PORT_ADDR *remote);

#endif // NET_TCP_IO_H