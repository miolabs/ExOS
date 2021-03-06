#include <net/tcp_io.h>
#include <comm/comm.h>
#include <kernel/tree.h>
#include <support/board_hal.h>
#include <support/apple/iap_comm.h>
#include <stdio.h>

#define SERVER_THREAD_STACK 2048
static EXOS_THREAD _thread;
static unsigned char _thread_stack[SERVER_THREAD_STACK];


#define TCP_BUFFER_SIZE 4096
static unsigned char _rcv_buffer[TCP_BUFFER_SIZE]; 
static unsigned char _snd_buffer[TCP_BUFFER_SIZE] __attribute__((section(".dma")));
static void *_server(void *);

static APPLE_IAP_PROTOCOL_MANAGER _manager = { .Name = "com.miolabs.lock" };

void lock_server_start()
{
	iap_comm_add_protocol(&_manager);
	exos_thread_create(&_thread, 0, _thread_stack, SERVER_THREAD_STACK, NULL, _server, NULL); 
}

static void *_server(void *arg)
{
	int err;
	int done, done2;
	int total;
	COMM_IO_ENTRY comm;
	COMM_IO_ENTRY iap_comm;
	static unsigned char buffer[1024];

	while(1)
	{
		int err;
		EXOS_TREE_DEVICE *dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_path(NULL, "dev/iap/com.miolabs.lock");
		if (dev_node != NULL)
		{
			comm_io_create(&iap_comm, dev_node->Device, dev_node->Unit, EXOS_IOF_WAIT);
			err = comm_io_open(&iap_comm);
			if (err == 0)
			{
				hal_led_set(0, 1);

				while(1)
				{
					done = exos_io_read((EXOS_IO_ENTRY *)&iap_comm, buffer, 1024);
					if (done < 0) break;

					long id = 0x12345678; 
					done = sprintf(buffer, "Server id: 0x%lx\r\n", id);
					done = exos_io_write((EXOS_IO_ENTRY *)&iap_comm, buffer, done);
				}

				hal_led_set(0, 0);
				comm_io_close(&iap_comm);
			}
		}
		exos_thread_sleep(100);
	}
}

