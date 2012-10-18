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
	EXOS_FIFO Incoming;

	unsigned short LocalWindowSize;
	unsigned short RemoteWindowSize;

	unsigned long RcvNext;
//	TCP_RECEIVE_CALLBACK RcvCallback;

	unsigned long SndBase;
	unsigned long SndAck;
	unsigned long SndNext; 
	NET_MBUF *SndBuffer;
	TCP_FLAGS SndFlags;
//	TCP_SEND_CALLBACK SndCallback;
} TCP_IO_ENTRY;

//typedef void (* TCP_RECEIVE_CALLBACK)(TCP_PCB *pcb, void *data, int data_length);
//typedef void (* TCP_SEND_CALLBACK)(TCP_PCB *pcb, NET_MBUF *done);


void __tcp_io_initialize();
TCP_IO_ENTRY *__tcp_io_find_io(ETH_ADAPTER *adapter, unsigned short local_port, IP_ADDR src_ip, unsigned short src_port);
void __tcp_io_remove_io(TCP_IO_ENTRY *io);

void net_tcp_create_io(TCP_IO_ENTRY *io, EXOS_IO_FLAGS flags);
int net_tcp_listen(TCP_IO_ENTRY *io);
int net_tcp_accept(TCP_IO_ENTRY *io);
int net_tcp_connect(TCP_IO_ENTRY *io, IP_PORT_ADDR *remote);

#endif // NET_TCP_IO_H