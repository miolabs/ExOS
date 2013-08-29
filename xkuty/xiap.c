#include "xiap.h"

#include <support/apple/iap.h>
#include <kernel/tree.h>
#include <comm/comm.h>
#include <CMSIS/lpc17xx.h>

static COMM_IO_ENTRY _comm;
static int _connected = 0;

static void _connect()
{
	int err;
	EXOS_TREE_DEVICE *dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_node(NULL, "dev/iap/com.miolabs.xkuty1");
	if (dev_node != NULL)
	{
		comm_io_create(&_comm, dev_node->Device, dev_node->Unit, EXOS_IOF_NONE);	// don't wait
		err = comm_io_open(&_comm);
		if (err == 0)
		{
			_connected = 1;
		}
	}
}

static void _disconnect()
{
	_connected = 0;
	
	// TODO: close io, but investigate the consequences of not doing it

	NVIC_SystemReset();	// FIXME: DIRTY HACK!
}

void xiap_send_frame(DASH_DATA *dash)
{
	int err;
	if (!_connected)
	{
		_connect();
	}
	else
	{
		XIAP_FRAME_TO_IOS buffer = (XIAP_FRAME_TO_IOS) { 
			.Magic = XIAP_MAGIC, 
			.Speed = dash->Speed, .StatusFlags = dash->CpuStatus,
			.Distance = dash->Distance,
			.Battery = dash->battery_level_fx8, 
			.DriveMode = dash->ActiveConfig.DriveMode };

		err = exos_io_write((EXOS_IO_ENTRY *)&_comm, &buffer, sizeof(buffer));
		if (err < 0)
			_disconnect();
	}
}

static int _from_ios_read = 0;
static XIAP_FRAME_FROM_IOS _from_ios_temp;

int xiap_get_frame(XIAP_FRAME_FROM_IOS *fromIOS)
{
	unsigned char* raw = (unsigned char *)&_from_ios_temp;

	if (_connected)
	{
		int requested = sizeof(_from_ios_temp) - _from_ios_read;
		int done = exos_io_read((EXOS_IO_ENTRY *)&_comm, raw, requested);
		if (done > 0)
		{
			_from_ios_read += done;
			if (_from_ios_read >= sizeof(_from_ios_temp))
			{
				*fromIOS = _from_ios_temp;
				_from_ios_read = 0;
				return 1;
			}
		}
		else if (done < 0) 
			_disconnect();
	}
	return 0;
}




