#include <usb/host.h>
#include <support/usb/driver/usbprint.h>
#include <support/usb/driver/ftdi.h>
#include <net/tcp_io.h>
#include <comm/comm.h>
#include <kernel/tree.h>
#include <support/board_hal.h>
#include <stdio.h>

COMM_IO_ENTRY _comm;
TCP_IO_ENTRY _socket;
unsigned char _buffer[1024];

#define TCP_BUFFER_SIZE 8192 // tiny window for stress
unsigned char _rcv_buffer[TCP_BUFFER_SIZE]; 
unsigned char _snd_buffer[TCP_BUFFER_SIZE] __attribute__((section(".dma")));

// NOTE: hook called by net stack
void net_board_set_mac_address(NET_ADAPTER *adapter, int index)
{
	adapter->MAC = (HW_ADDR) { 0x00, 0x18, 0x1b, 0x05, 0x05, 0x06 };
}

void main()
{
	usb_host_initialize();
	int err = 0;

	NET_ADAPTER *adapter = NULL;
	if (net_adapter_enum(&adapter))
	{
//		adapter->IP = (IP_ADDR) { 10, 0, 1, 10 };
//		adapter->NetMask = (IP_ADDR) { 255, 255, 255, 0 };
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
			dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_node(NULL, "dev/usbftdi");
		if (dev_node == NULL)
			dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_node(NULL, "dev/comm0");

		if (dev_node != NULL)
		{
			comm_io_create(&_comm, dev_node->Device, dev_node->Unit, EXOS_IOF_WAIT); 
			err = comm_io_open(&_comm);
			if (err == 0)
			{
				while(1)
				{
					int done = exos_io_read((EXOS_IO_ENTRY *)&_socket, _buffer, 1024);
					if (done < 0) break;

hal_led_set(1, 1);
					done = exos_io_write((EXOS_IO_ENTRY *)&_socket, _buffer, done);
//					done = exos_io_write((EXOS_IO_ENTRY *)&_comm, _buffer, done);
hal_led_set(1, 0);
					if (done < 0) break;
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
}
