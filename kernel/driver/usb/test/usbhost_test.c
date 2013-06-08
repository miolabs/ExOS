#include <usb/host.h>
#include <support/usb/driver/usbprint.h>
#include <support/usb/driver/ftdi.h>
#include <support/usb/driver/iap2.h>
#include <support/usb/driver/hid.h>
#include <net/tcp_io.h>
#include <comm/comm.h>
#include <kernel/tree.h>
#include <support/board_hal.h>
#include <stdio.h>
#include <support/misc/apple_cp20.h>

COMM_IO_ENTRY _comm;
TCP_IO_ENTRY _socket;
unsigned char _buffer[1024];

#define TCP_BUFFER_SIZE 8192 // tiny window for stress
unsigned char _rcv_buffer[TCP_BUFFER_SIZE]; 
unsigned char _snd_buffer[TCP_BUFFER_SIZE] __attribute__((section(".dma")));

void main()
{
	apple_cp20_initialize(0);
	usb_host_initialize();
	int err = 0;
	int done = 0;

	NET_ADAPTER *adapter = NULL;
	if (net_adapter_enum(&adapter))
	{
		sprintf(_buffer, "%d.%d.%d.%d", adapter->IP.Bytes[0], adapter->IP.Bytes[1], adapter->IP.Bytes[2], adapter->IP.Bytes[3]);
	}

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

		EXOS_TREE_DEVICE *dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_node(NULL, "dev/usbprint");
		if (dev_node == NULL)
			dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_node(NULL, "dev/usbftdi0");
		if (dev_node == NULL)
			dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_node(NULL, "dev/comm0");


		if (dev_node != NULL)
		{
			comm_io_create(&_comm, dev_node->Device, dev_node->Unit, EXOS_IOF_WAIT);
			err = comm_io_open(&_comm);
			if (err == 0)
			{
				done = sprintf(_buffer, "Connected to '%s'!\r\n", dev_node->Name);
				done = exos_io_write((EXOS_IO_ENTRY *)&_socket, _buffer, done);
				
				while(1)
				{
					EXOS_EVENT *input_events[] = { &_socket.InputEvent, &_comm.InputEvent };
					done = exos_event_wait_multiple(input_events, 2, EXOS_TIMEOUT_NEVER);

					if (_socket.InputEvent.State)
					{
						done = exos_io_read((EXOS_IO_ENTRY *)&_socket, _buffer, 1024);
						if (done < 0) break;
	
						done = exos_io_write((EXOS_IO_ENTRY *)&_comm, _buffer, done);
						if (done < 0) break;
					}

					if (_comm.InputEvent.State)
					{
						done = exos_io_read((EXOS_IO_ENTRY *)&_comm, _buffer, 1024);
						if (done < 0) break;

						done = exos_io_write((EXOS_IO_ENTRY *)&_socket, _buffer, done);
						if (done < 0) break;
					}
				}

				comm_io_close(&_comm);
			}
		}
			
		hal_led_set(0, 0);
		net_io_close((NET_IO_ENTRY *)&_socket, &buffers);
	}
}

void usb_host_add_drivers()
{
    usbprint_initialize();
	ftdi_initialize();
	iap2_initialize();
	usbd_hid_initialize();
}
