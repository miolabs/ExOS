#include <net/tcp_io.h>
#include <comm/comm.h>
#include <kernel/tree.h>
#include <support/board_hal.h>
#include <stdio.h>

TCP_IO_ENTRY _socket;
unsigned char _buffer[1024];

#define TCP_BUFFER_SIZE 32
unsigned char _rcv_buffer[TCP_BUFFER_SIZE];
unsigned char _snd_buffer[TCP_BUFFER_SIZE];

COMM_IO_ENTRY _comm;

int main()
{
	int err;

	EXOS_TREE_DEVICE *dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_node(NULL, "dev/comm0");
	comm_io_create(&_comm, dev_node->Device, dev_node->Port, EXOS_IOF_WAIT); 
	err = comm_io_open(&_comm, 115200);

	net_tcp_io_create(&_socket, EXOS_IOF_WAIT, _rcv_buffer, TCP_BUFFER_SIZE, _snd_buffer, TCP_BUFFER_SIZE);
	
	IP_PORT_ADDR local = { .Port = 23 };
	err = net_io_bind((NET_IO_ENTRY *)&_socket, &local);

	err = net_tcp_listen(&_socket);

	err = net_tcp_accept(&_socket);

	hal_led_set(0, 1);

	while(1)
	{
		int done = exos_io_read((EXOS_IO_ENTRY *)&_socket, _buffer, 1024); 

		int done2 = exos_io_write((EXOS_IO_ENTRY *)&_comm, _buffer, done);

		int done3 = exos_io_write((EXOS_IO_ENTRY *)&_socket, _buffer, done);

		if (done != done2 || done != done3)
		{
			exos_thread_sleep(1);
		}
	}
}


