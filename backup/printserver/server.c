#include <net/tcp_io.h>
#include <comm/comm.h>
#include <kernel/tree.h>
#include <support/board_hal.h>
#include <stdio.h>

#define SERVER_THREAD_STACK 2048
static EXOS_THREAD _thread0;
static unsigned char _thread_stack0[SERVER_THREAD_STACK];
static EXOS_THREAD _thread1;
static unsigned char _thread_stack1[SERVER_THREAD_STACK];

typedef struct
{
	unsigned char *RcvBuffer;
	unsigned char *SndBuffer;
	unsigned char *PortName;
	unsigned short Port;
} _SERVER_CFG;

#define TCP_BUFFER_SIZE 4096
static unsigned char _rcv_buffer0[TCP_BUFFER_SIZE]; 
static unsigned char _snd_buffer0[TCP_BUFFER_SIZE] __attribute__((section(".dma")));
static _SERVER_CFG _cfg0 = { .RcvBuffer = _rcv_buffer0, .SndBuffer = _snd_buffer0, "dev/usbftdi0", .Port = 9000 };
static unsigned char _rcv_buffer1[TCP_BUFFER_SIZE]; 
static unsigned char _snd_buffer1[TCP_BUFFER_SIZE] __attribute__((section(".dma")));
static _SERVER_CFG _cfg1 = { .RcvBuffer = _rcv_buffer1, .SndBuffer = _snd_buffer1, "dev/usbftdi1", .Port = 9001 };
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
	int done, done2;
	int total;
	COMM_IO_ENTRY comm;
	TCP_IO_ENTRY socket;
	static unsigned char buffer[1024];

	while(1)
	{
		EXOS_IO_STREAM_BUFFERS buffers = (EXOS_IO_STREAM_BUFFERS) {
			.RcvBuffer = cfg->RcvBuffer, .RcvBufferSize = TCP_BUFFER_SIZE,
			.SndBuffer = cfg->SndBuffer, .SndBufferSize = TCP_BUFFER_SIZE };

		net_tcp_io_create(&socket, EXOS_IOF_WAIT);

		IP_PORT_ADDR local = (IP_PORT_ADDR) { .Address = IP_ADDR_ANY, .Port = cfg->Port };
		err = net_io_bind((NET_IO_ENTRY *)&socket, &local); 
		err = net_io_listen((NET_IO_ENTRY *)&socket);

		err = net_io_accept((NET_IO_ENTRY *)&socket, (NET_IO_ENTRY *)&socket, &buffers);
		hal_led_set(0, 1);
		
		exos_io_set_timeout((EXOS_IO_ENTRY *)&socket, 2000); // NOTE: we didn't set timeout before because we don't want accept()  to timeout

		EXOS_TREE_DEVICE *dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_path(NULL, "dev/usbprint");
		if (dev_node == NULL)
			dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_path(NULL, cfg->PortName);
		if (dev_node == NULL)
			dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_path(NULL, "dev/comm0");

		if (dev_node != NULL)
		{
			comm_io_create(&comm, dev_node->Device, dev_node->Unit, EXOS_IOF_WAIT); 
			err = comm_io_open(&comm);
			if (err == 0)
			{
				total = 0;
#ifdef DEBUG
				done = sprintf(buffer, "Connection accepted:\r\n");
				done = exos_io_write((EXOS_IO_ENTRY *)&socket, buffer, done);				
#endif
				
				while(1)
				{
					done = exos_io_read((EXOS_IO_ENTRY *)&socket, buffer, 1024);
					if (done < 0) break;

hal_led_set(1, 1);
					total += done;
					done2 = exos_io_write((EXOS_IO_ENTRY *)&comm, buffer, done);
hal_led_set(1, 0);
					if (done2 < 0) break;
				}

#ifdef DEBUG
				int done3 = sprintf(buffer, "Connection closed: %d bytes\r\n", total);
				done3 = exos_io_write((EXOS_IO_ENTRY *)&socket, buffer, done3);
#endif

				exos_thread_sleep(100);
				comm_io_close(&comm);
			}
		}
			
		hal_led_set(0, 0);
		net_io_close((NET_IO_ENTRY *)&socket, &buffers);
	}
}

