#include <usb/host.h>
#include <support/usb/driver/usbprint.h>
#include <support/usb/driver/ftdi.h>
#include <net/adapter.h>
#include "discovery.h"
#include "server.h"

// NOTE: hook called by net stack
void net_board_set_mac_address(NET_ADAPTER *adapter, int index)
{
	adapter->MAC = (HW_ADDR) { 0x00, 0x18, 0x1b, 0x05, 0x05, 0x06 };
}

void main()
{
	usb_host_initialize();
	int err = 0;

	server_start();

	discovery_loop();
	// we should reset upon exit
}

void usb_host_add_drivers()
{
    usbprint_initialize();
	ftdi_initialize();
}