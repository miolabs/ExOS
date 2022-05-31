#include "discovery.h"
#include <net/udp_io.h>
#include <net/adapter.h>

#define MAGIC(a, b, c, d) (((a) << 24)|((b) << 16)|((c) << 8)|(d))
#define MAGIC_CMD MAGIC('E', 'X', 'O', 'C')
#define MAGIC_REPLY MAGIC('E', 'X', 'O', 'R')

static UDP_IO_ENTRY _io;
static unsigned char _frame_buffer[256];
static int _quit;
static 	DISCOVERY_CONFIG _cfg;

static int _parse(DISCOVERY_MSG *msg, NET_ADAPTER *adapter, const DISCOVERY_CONFIG_CALLBACK *callback);
static int _fill_reply(DISCOVERY_CMD cmd, void *data, int data_length);
static int _fill_config(DISCOVERY_CMD cmd, const DISCOVERY_CONFIG_CALLBACK *callback);

void discovery_loop(const DISCOVERY_CONFIG_CALLBACK *callback)
{
	net_udp_io_create(&_io, EXOS_IOF_WAIT);
	exos_io_set_timeout((EXOS_IO_ENTRY *)&_io, 10000);

	IP_PORT_ADDR local = (IP_PORT_ADDR) { .Address = IP_ADDR_ANY, .Port = DISCOVERY_PORT };
	int done = net_io_bind((NET_IO_ENTRY *)&_io, &local);

	IP_PORT_ADDR remote;
	_quit = 0;
	while(!_quit)
	{
		done = net_io_receive((NET_IO_ENTRY *)&_io, _frame_buffer, 256, &remote);
		if (done >= (signed)sizeof(DISCOVERY_MSG))
		{
			NET_ADAPTER *adapter = net_adapter_find(remote.Address);
			int reply = _parse((DISCOVERY_MSG *)_frame_buffer, adapter, callback);
			if (reply > 0)
			{
				net_io_send((NET_IO_ENTRY *)&_io, _frame_buffer, reply, &remote);
			}
		}
	}
}

static int _parse(DISCOVERY_MSG *msg, NET_ADAPTER *adapter, const DISCOVERY_CONFIG_CALLBACK *callback)
{
	if (msg->Magic == MAGIC_CMD)
	{
		switch(msg->Command)
		{
			case DISCOVERY_CMD_DISCOVER:
				_cfg = (DISCOVERY_CONFIG) { .IP = adapter->IP.Value, .Mask = adapter->NetMask.Value };
				return _fill_reply(DISCOVERY_CMD_READY, &_cfg, sizeof(DISCOVERY_CONFIG));
			case DISCOVERY_CMD_QUIT:
				_quit = 1;
				break;
//			case DISCOVERY_CMD_RECONFIG:
//				__config.Network = *(NET_CONFIG *)msg->Data;
//				return _fill_reply(DISCOVERY_CMD_READY,
//					&__config.Network, sizeof(NET_CONFIG));
		}
	}
	return 0;
}

static int _fill_reply(DISCOVERY_CMD cmd, void *data, int data_length)
{
	DISCOVERY_MSG *msg = (DISCOVERY_MSG *)_frame_buffer;
	*msg = (DISCOVERY_MSG) {
		.Magic = MAGIC_REPLY,
		.Command = cmd };
	
	int length = sizeof(DISCOVERY_MSG);
	if (data != NULL)
	{
		for (int i = 0; i < data_length; i++) msg->Data[i] = ((unsigned char *)data)[i];
		length += data_length;
	}
	return length;
}

static int _fill_config(DISCOVERY_CMD cmd, const DISCOVERY_CONFIG_CALLBACK *callback)
{
	DISCOVERY_MSG *msg = (DISCOVERY_MSG *)_frame_buffer;
	*msg = (DISCOVERY_MSG) {
		.Magic = MAGIC_REPLY,
		.Command = cmd };
	
	int cfg_len = callback->FillConfig(msg->Data);
	return sizeof(DISCOVERY_MSG) + cfg_len;
}





