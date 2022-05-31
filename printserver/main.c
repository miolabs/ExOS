#include <usb/host.h>
#include <support/usb/driver/usbprint.h>
#include <support/usb/driver/ftdi.h>
#include <net/adapter.h>
#include "discovery.h"
#include "server.h"

#ifdef BOARD_E2468
// NOTE: hook called by net stack
void net_board_set_mac_address(NET_ADAPTER *adapter, int index)
{
        // MAC 1 (0:18:1b:5:1c:13)
	adapter->MAC = (HW_ADDR) { 0x00, 0x18, 0x1b, 0x05, 0x1c, 0x13 };
        // MAC 2 (0:1:38:38:a8:20)
	// adapter->MAC = (HW_ADDR) { 0x00, 0x1, 0x38, 0x38, 0xa8, 0x20 };
        // MAC 3 (0:14:bf:71:ac:88)
        //adapter->MAC = (HW_ADDR) { 0x00, 0x14, 0xbf, 0x71, 0xac, 0x88 };  
}
#endif

void main()
{
	usb_host_initialize();
	int err = 0;

#ifdef BOARD_E2468
	NET_ADAPTER *adapter = NULL;
	if (net_adapter_enum(&adapter))
	{
		adapter->IP = (IP_ADDR) { 10, 0, 1, 10 };
	}
#endif
	server_start();

	discovery_loop();
	// we should reset upon exit
}

void usb_host_add_drivers()
{
    usbprint_initialize();
	ftdi_initialize();
}
