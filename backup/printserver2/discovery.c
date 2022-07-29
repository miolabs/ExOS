#include "discovery.h"
#include "config.h"
#include <net/udp_io.h>

#define MAGIC(a, b, c, d) (((a) << 24)|((b) << 16)|((c) << 8)|(d))
#define MAGIC_CMD MAGIC('M', 'I', 'O', 'D')
#define MAGIC_REPLY MAGIC('M', 'I', 'O', 'R')

static UDP_IO_ENTRY _io;
static unsigned char _frame_buffer[256];
static int _reset;

static int _parse(int length);
static int _fill_reply(SERVER_DISCOVERY_CMD cmd, void *data, int data_length);

void discovery_loop()
{
	net_udp_io_create(&_io, EXOS_IOF_WAIT);
	exos_io_set_timeout((EXOS_IO_ENTRY *)&_io, 20);

	IP_PORT_ADDR local = (IP_PORT_ADDR) { .Address = IP_ADDR_ANY, .Port = 18180 };
	int done = net_io_bind((NET_IO_ENTRY *)&_io, &local);

	IP_PORT_ADDR remote;
	_reset = 0;
	while(!_reset)
	{
		done = net_io_receive((NET_IO_ENTRY *)&_io, _frame_buffer, 256, &remote);
		if (done > 0)
		{
			int reply = _parse(done);
			if (reply > 0)
			{
				net_io_send((NET_IO_ENTRY *)&_io, _frame_buffer, reply, &remote);
			}
		}
	}
}

static int _parse(int length)
{
	SERVER_DISCOVERY_MSG *msg = (SERVER_DISCOVERY_MSG *)_frame_buffer;
	if (msg->Magic == MAGIC_CMD)
	{
		switch(msg->Command)
		{
			case SERVER_DISCOVERY_DISCOVER:
				return _fill_reply(SERVER_DISCOVERY_READY,
					&__config.Network, sizeof(NET_CONFIG));
			case SERVER_DISCOVERY_RESET:
				_reset = 1;
				break;
			case SERVER_DISCOVERY_RECONFIG:
				__config.Network = *(NET_CONFIG *)msg->Data;
				return _fill_reply(SERVER_DISCOVERY_READY,
					&__config.Network, sizeof(NET_CONFIG));
		}
	}
	return 0;
}

static int _fill_reply(SERVER_DISCOVERY_CMD cmd, void *data, int data_length)
{
	SERVER_DISCOVERY_MSG *msg = (SERVER_DISCOVERY_MSG *)_frame_buffer;
	*msg = (SERVER_DISCOVERY_MSG) {
		.Magic = MAGIC_REPLY,
		.Command = cmd };
	
	int length = sizeof(SERVER_DISCOVERY_MSG);
	if (data != NULL)
	{
		for (int i = 0; i < data_length; i++) msg->Data[i] = ((unsigned char *)data)[i];
		length += data_length;
	}
	return length;
}





