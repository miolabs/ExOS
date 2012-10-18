#include <net/tcp_io.h>

TCP_IO_ENTRY _socket;

int main()
{
	int err;

	net_tcp_create_io(&_socket, EXOS_IOF_WAIT);

	IP_PORT_ADDR local = { .Port = 23 };
	err = net_io_bind((NET_IO_ENTRY *)&_socket, &local);

	err = net_tcp_listen(&_socket);

	err = net_tcp_accept(&_socket);

	while(1);
}


