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

static APPLE_IAP_PROTOCOL_MANAGER _manager = { .Name = "com.miolabs.serial" };

void iap_server_start()
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
		sprintf(buffer, "/dev/iap/%s", _manager.Name);
		if (comm_io_create_from_path(&iap_comm, buffer, EXOS_IOF_WAIT))
		{
			err = comm_io_open(&iap_comm);
			if (err == 0)
			{
				printf("iAP server: connection opened for protocol '%s'\r\n", _manager.Name);

				hal_led_set(0, 1);

				int comm_ready = comm_io_create_from_path(&comm, "dev/usbprint", EXOS_IOF_WAIT);
				if (!comm_ready)
					comm_ready = comm_io_create_from_path(&comm, "dev/usbftdi0", EXOS_IOF_WAIT);
				
				if (comm_ready)
				{
					if (0 != comm_io_open(&comm))
						comm_ready = 0;
				}
				
				if (!comm_ready)
				{
					printf("iAP server: output stream could not be opened; output will be flushed\r\n");
				}

		
				total = 0;
				while(1)
				{
					done = exos_io_read((EXOS_IO_ENTRY *)&iap_comm, buffer, 1024);
					if (done < 0) break;

					if (done > 0)
					{
						printf("iAP server: %d bytes received\r\n", done); 
						total += done;
						if (comm_ready)
						{
							done2 = exos_io_write((EXOS_IO_ENTRY *)&comm, buffer, done);
							if (done2 < 0) break;
						}
					}
				}

				printf("iAP server: connection closed (total bytes = %d)!\r\n", total);
	
				if (comm_ready)
					comm_io_close(&comm);

				hal_led_set(0, 0);
				comm_io_close(&iap_comm);
			}
		}
		exos_thread_sleep(100);
	}
}

