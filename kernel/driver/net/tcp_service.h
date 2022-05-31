#ifndef NET_TCP_SERVICE_H
#define NET_TCP_SERVICE_H

#include <net/tcp_io.h>

void net_tcp_service_start();
TCP_IO_ENTRY *__tcp_io_find_io(unsigned short local_port, IP_ADDR remote_ip, unsigned short remote_port);
void __tcp_io_remove_io(TCP_IO_ENTRY *io);
TCP_INCOMING_CONN *__tcp_get_incoming_conn();

int net_tcp_listen(TCP_IO_ENTRY *io, unsigned short local_port);
int net_tcp_accept(TCP_IO_ENTRY *io, const EXOS_IO_STREAM_BUFFERS *buffers, TCP_INCOMING_CONN *conn);
int net_tcp_connect(TCP_IO_ENTRY *io, const EXOS_IO_STREAM_BUFFERS *buffers, IP_PORT_ADDR *remote);
int net_tcp_close(TCP_IO_ENTRY *io);
void net_tcp_service(TCP_IO_ENTRY *io, int wait);

#endif // NET_TCP_SERVICE_H


