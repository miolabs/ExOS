#ifndef NET_TCP_SERVICE_H
#define NET_TCP_SERVICE_H

#include <net/tcp_io.h>

#ifndef TCP_SERVICE_THREAD_STACK
#define TCP_SERVICE_THREAD_STACK 512
#endif

void net_tcp_service_start();
TCP_IO_ENTRY *__tcp_io_find_io(ETH_ADAPTER *adapter, unsigned short local_port, IP_ADDR src_ip, unsigned short src_port);
void __tcp_io_remove_io(TCP_IO_ENTRY *io);
int __tcp_send(TCP_IO_ENTRY *io);

int net_tcp_bind(TCP_IO_ENTRY *io, unsigned short local_port, ETH_ADAPTER *adapter);
void net_tcp_service(TCP_IO_ENTRY *io, int wait);

#endif // NET_TCP_SERVICE_H


