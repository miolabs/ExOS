#include <net/tcp_io.h>
#include <comm/comm.h>
#include <kernel/tree.h>
#include <support/board_hal.h>
#include <stdio.h>

#define SERVER_THREAD_STACK 1024
static EXOS_THREAD _thread;
static unsigned char _thread_stack[SERVER_THREAD_STACK];

COMM_IO_ENTRY _comm;
TCP_IO_ENTRY _socket;
static unsigned char _buffer[1024] ;

#define TCP_BUFFER_SIZE 8192 // tiny window for stress
static unsigned char _rcv_buffer[TCP_BUFFER_SIZE]; 
static unsigned char _snd_buffer[TCP_BUFFER_SIZE] __attribute__((section(".dma")));
static void *_server(void *);

void server_start()
{
	exos_thread_create(&_thread, 0, _thread_stack, SERVER_THREAD_STACK, NULL, _server, NULL); 
}

static void *_server(void *arg)
{
	int err;
	int done;
	int total;

	while(1)
	{
		EXOS_IO_STREAM_BUFFERS buffers = (EXOS_IO_STREAM_BUFFERS) {
			.RcvBuffer = _rcv_buffer, .RcvBufferSize = TCP_BUFFER_SIZE,
			.SndBuffer = _snd_buffer, .SndBufferSize = TCP_BUFFER_SIZE };

		net_tcp_io_create(&_socket, EXOS_IOF_WAIT);

		IP_PORT_ADDR local = (IP_PORT_ADDR) { .Address = IP_ADDR_ANY, .Port = 23 };
		err = net_io_bind((NET_IO_ENTRY *)&_socket, &local); 
		err = net_io_listen((NET_IO_ENTRY *)&_socket);

		err = net_io_accept((NET_IO_ENTRY *)&_socket, (NET_IO_ENTRY *)&_socket, &buffers);
		hal_led_set(0, 1);
		
		exos_io_set_timeout((EXOS_IO_ENTRY *)&_socket, 2000); // NOTE: we didn't set timeout before because we don't want accept()  to timeout

		EXOS_TREE_DEVICE *dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_node(NULL, "dev/usbprint");
		if (dev_node == NULL)
			dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_node(NULL, "dev/usbftdi");
		if (dev_node == NULL)
			dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_node(NULL, "dev/comm0");

		if (dev_node != NULL)
		{
			comm_io_create(&_comm, dev_node->Device, dev_node->Unit, EXOS_IOF_WAIT); 
			err = comm_io_open(&_comm);
			if (err == 0)
			{
				total = 0;
#ifdef DEBUG
				done = sprintf(_buffer, "Connection accepted: %d bytes\r\n", total);
				done = exos_io_write((EXOS_IO_ENTRY *)&_comm, _buffer, done);				
#endif
				
				while(1)
				{
					int done = exos_io_read((EXOS_IO_ENTRY *)&_socket, _buffer, 1024);
					if (done < 0) break;

hal_led_set(1, 1);
					total += done;
					done = exos_io_write((EXOS_IO_ENTRY *)&_comm, _buffer, done);
hal_led_set(1, 0);
					if (done < 0) break;
				}

#ifdef DEBUG
				done = sprintf(_buffer, "Connection closed: %d bytes\r\n", total);
				done = exos_io_write((EXOS_IO_ENTRY *)&_comm, _buffer, done);
#endif

				exos_thread_sleep(100);
				comm_io_close(&_comm);
			}
		}
			
		hal_led_set(0, 0);
		net_io_close((NET_IO_ENTRY *)&_socket, &buffers);
	}
}

