#include <usb/host.h>
#include <support/usb/driver/usbprint.h>
#include <support/usb/driver/ftdi.h>
#include <net/adapter.h>
#include <support/services/discovery/discovery.h>
#include "server.h"
#include "relay.h"

static EXOS_PORT _port;
static EXOS_FIFO _free_msgs;
#define MSG_QUEUE 1
static RELAY_MSG _msg[MSG_QUEUE];

void main()
{
	usb_host_initialize();
	relay_initialize();

	NET_ADAPTER *adapter = NULL;
	if (net_adapter_enum(&adapter))
	{
//		adapter->IP = (IP_ADDR) { 192, 168, 1, 5 };
	}

	exos_port_create(&_port, NULL);
	exos_fifo_create(&_free_msgs, NULL);
	for(int i = 0; i < MSG_QUEUE; i++) exos_fifo_queue(&_free_msgs, (EXOS_NODE *)&_msg[i]);

	server_start();

	discovery_loop();
}

void usb_host_add_drivers()
{
    usbprint_initialize();
	ftdi_initialize();
}
