#include <usb/host.h>
#include <support/usb/driver/usbprint.h>
#include <support/usb/driver/ftdi.h>
#include <support/apple/iap.h>
#include <support/usb/driver/hid.h>
#include <net/adapter.h>
#include "iap_server.h"
#include "lock_server.h"
#include <stdio.h>

void main()
{
	printf("Starting USB...\r\n");

	usb_host_initialize();
	int err = 0;

	iap_server_start();
	lock_server_start();

	while(1)
	{
		exos_thread_sleep(100);
	}
}

void usb_host_add_drivers()
{
    usbprint_initialize();
	ftdi_initialize();
	usbd_hid_initialize();
	iap_initialize();
}
