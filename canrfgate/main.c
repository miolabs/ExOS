#include <usb/host.h>
#include <support/usb/driver/usbprint.h>
#include <support/usb/driver/ftdi.h>
#include <net/adapter.h>
//#include "discovery.h"
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

//	NET_ADAPTER *adapter = NULL;
//	if (net_adapter_enum(&adapter))
//	{
//		adapter->IP = (IP_ADDR) { 192, 168, 1, 5 };
//	}

	exos_port_create(&_port, NULL);
	exos_fifo_create(&_free_msgs, NULL);
	for(int i = 0; i < MSG_QUEUE; i++) exos_fifo_queue(&_free_msgs, (EXOS_NODE *)&_msg[i]);

	server_start();

	while(1) 
	{
		RELAY_MSG *msg = (RELAY_MSG *)exos_port_get_message(&_port, 1000);
		if (msg != NULL)
		{
			relay_set(msg->Unit, msg->RelayMask, msg->Time);
			exos_fifo_queue(&_free_msgs, (EXOS_NODE *)msg);
		}
	}
}

int open_relay(int unit, unsigned long mask, unsigned long time)
{
	RELAY_MSG *msg = (RELAY_MSG *)exos_fifo_dequeue(&_free_msgs);
	if (msg != NULL)
	{
		msg->Time = time;
		msg->Unit = unit;
		msg->RelayMask = mask;
		exos_port_send_message(&_port, (EXOS_MESSAGE *)msg);
		return 1;
	}
	return 0;
}

void usb_host_add_drivers()
{
    usbprint_initialize();
	ftdi_initialize();
}
