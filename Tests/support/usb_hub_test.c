#include <usb/host.h>
#include <support/usb/driver/usbprint.h>
#include <support/usb/driver/ftdi.h>
#include <support/usb/driver/hid.h>
#include <support/usb/driver/usbkb.h>
#include <support/usb/driver/usbhub.h>
#include <comm/comm.h>
#include <kernel/tree.h>
#include <support/board_hal.h>
#include <stdio.h>
#include <support/apple/iap.h>
#include <support/services/debug.h>

COMM_IO_ENTRY _comm;
unsigned char _buffer[1024];

void main()
{
	printf("Initializing usb...\r\n");
	usb_host_initialize();

	COMM_IO_ENTRY kb_io;
	while(!comm_io_create_from_path(&kb_io, "dev/usbkb0", EXOS_IOF_WAIT))
		exos_thread_sleep(100);

	while(1)
	{
		if (comm_io_open(&kb_io) == 0)
		{
			while(1)
			{
				static char buffer[64];
				int done = exos_io_read((EXOS_IO_ENTRY *)&kb_io, buffer, sizeof(buffer));
				if (done < 0) break;

				debug_print(buffer, done);
			}

			debug_printf("\r\nClosed!\r\n");
			comm_io_close(&kb_io);
		}

		exos_thread_sleep(100);
	}
}

void usb_host_add_drivers()
{
    usbprint_initialize();
	ftdi_initialize();
	
	usbd_hub_initialize();
	usbd_hid_initialize();
	usbkb_initialize();

	iap_initialize();
}
