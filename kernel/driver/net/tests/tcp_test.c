
#if 1
#include <net/tcp_io.h>
#include <comm/comm.h>
#include <kernel/tree.h>
#include <support/board_hal.h>

#include "support/dm36x/vpbe.h"

#include <stdio.h>


int main()
{
	VPBE_SIMPLE_SPEC nil;

	vpbe_initialize_simple  ( &nil);

	hal_board_init_pinmux(HAL_RESOURCE_TVOUT, 0);

	while (1)
	{
	}
}
#endif

#if 0
TCP_IO_ENTRY _socket;
unsigned char _buffer[1024];

#define TCP_BUFFER_SIZE 32 // tiny window for stress
unsigned char _rcv_buffer[TCP_BUFFER_SIZE];
unsigned char _snd_buffer[TCP_BUFFER_SIZE];

COMM_IO_ENTRY _comm;

int main()
{
	int err;
	EXOS_IO_ENTRY *comm = NULL;
	EXOS_TREE_DEVICE *dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_node(NULL, "dev/comm0");
	if (dev_node != NULL)
	{
		comm_io_create(&_comm, dev_node->Device, dev_node->Unit, EXOS_IOF_WAIT); 
		err = comm_io_open(&_comm);	// FIXME: set baudrate to 115200
		if (err == 0) comm = (EXOS_IO_ENTRY *)&_comm;
	}

	while(1)
	{
		net_tcp_io_create(&_socket, EXOS_IOF_WAIT);
	
		IP_PORT_ADDR local = (IP_PORT_ADDR) { .Address = IP_ADDR_ANY, .Port = 23 };
		err = net_io_bind((NET_IO_ENTRY *)&_socket, &local); 
		err = net_io_listen((NET_IO_ENTRY *)&_socket);

		EXOS_IO_STREAM_BUFFERS buffers = (EXOS_IO_STREAM_BUFFERS) {
			.RcvBuffer = _rcv_buffer, .RcvBufferSize = TCP_BUFFER_SIZE,
			.SndBuffer = _snd_buffer, .SndBufferSize = TCP_BUFFER_SIZE };
		err = net_io_accept((NET_IO_ENTRY *)&_socket, (NET_IO_ENTRY *)&_socket, &buffers);
	
		hal_led_set(0, 1);
	
		while(1)
		{
			int done = exos_io_read((EXOS_IO_ENTRY *)&_socket, _buffer, 1024); 
			if (done < 0) break;
	
			comm != NULL ? exos_io_write(comm, _buffer, done) : done;
	
			exos_io_write((EXOS_IO_ENTRY *)&_socket, _buffer, done);
		}
	
		hal_led_set(0, 0);
		net_io_close((NET_IO_ENTRY *)&_socket, &buffers);
	}

	if (comm != NULL) comm_io_close((COMM_IO_ENTRY *)comm);
}
#endif

