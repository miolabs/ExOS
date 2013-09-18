#include <net/tcp_io.h>
#include <comm/comm.h>
#include <kernel/tree.h>
#include <support/board_hal.h>
#include <stdio.h>

#define SERVER_THREAD_STACK 1024
static EXOS_THREAD _thread0;
static unsigned char _thread_stack0[SERVER_THREAD_STACK];
static EXOS_THREAD _thread1;
static unsigned char _thread_stack1[SERVER_THREAD_STACK];

COMM_IO_ENTRY _comm;
TCP_IO_ENTRY _socket;
static unsigned char _buffer[1024] ;

typedef struct
{
	unsigned char *RcvBuffer;
	unsigned char *SndBuffer;
	unsigned short Port;
} _SERVER_CFG;

#define TCP_BUFFER_SIZE 4096
static unsigned char _rcv_buffer0[TCP_BUFFER_SIZE]; 
static unsigned char _snd_buffer0[TCP_BUFFER_SIZE] __attribute__((section(".dma")));
static _SERVER_CFG _cfg0 = { .RcvBuffer = _rcv_buffer0, .SndBuffer = _snd_buffer0, .Port = 9000 };
static unsigned char _rcv_buffer1[TCP_BUFFER_SIZE]; 
static unsigned char _snd_buffer1[TCP_BUFFER_SIZE] __attribute__((section(".dma")));
static _SERVER_CFG _cfg1 = { .RcvBuffer = _rcv_buffer1, .SndBuffer = _snd_buffer1, .Port = 9001 };
static void *_server(void *);

void server_start()
{
	exos_thread_create(&_thread0, 0, _thread_stack0, SERVER_THREAD_STACK, NULL, _server, &_cfg0); 
	exos_thread_create(&_thread1, 0, _thread_stack1, SERVER_THREAD_STACK, NULL, _server, &_cfg1); 
}

static void *_server(void *arg)
{
	_SERVER_CFG *cfg = (_SERVER_CFG *)arg;
	int err;
	int done;
	int total;

	while(1)
	{
		EXOS_IO_STREAM_BUFFERS buffers = (EXOS_IO_STREAM_BUFFERS) {
			.RcvBuffer = cfg->RcvBuffer, .RcvBufferSize = TCP_BUFFER_SIZE,
			.SndBuffer = cfg->SndBuffer, .SndBufferSize = TCP_BUFFER_SIZE };

		net_tcp_io_create(&_socket, EXOS_IOF_WAIT);

		IP_PORT_ADDR local = (IP_PORT_ADDR) { .Address = IP_ADDR_ANY, .Port = cfg->Port };
		err = net_io_bind((NET_IO_ENTRY *)&_socket, &local); 
		err = net_io_listen((NET_IO_ENTRY *)&_socket);

		err = net_io_accept((NET_IO_ENTRY *)&_socket, (NET_IO_ENTRY *)&_socket, &buffers);
		hal_led_set(0, 1);
		
		exos_io_set_timeout((EXOS_IO_ENTRY *)&_socket, 2000); // NOTE: we didn't set timeout before because we don't want accept()  to timeout

		EXOS_TREE_DEVICE *dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_path(NULL, "dev/usbprint");
		if (dev_node == NULL)
			dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_path(NULL, "dev/usbftdi0");
		if (dev_node == NULL)
			dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_path(NULL, "dev/comm0");

		if (dev_node != NULL)
		{
			comm_io_create(&_comm, dev_node->Device, dev_node->Unit, EXOS_IOF_WAIT); 
			err = comm_io_open(&_comm);
			if (err == 0)
			{
				total = 0;
#ifdef DEBUG
				done = sprintf(_buffer, "Connection accepted:\r\n");
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

