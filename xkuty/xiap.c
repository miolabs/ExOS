#include "xiap.h"

#include <support/apple/iap.h>
#include <kernel/tree.h>
#include <comm/comm.h>

static COMM_IO_ENTRY _comm;
static int _connected = 0;

void xiap_send_frame(DASH_DATA *dash)
{
	int err;
	if (!_connected)
	{
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
	else
	{
		XIAP_FRAME buffer = (XIAP_FRAME) { .Magic = XIAP_MAGIC, .Speed = dash->Speed, .Distance = dash->Distance };
		err = exos_io_write((EXOS_IO_ENTRY *)&_comm, &buffer, sizeof(buffer));
		if (err < 0) _connected = 0;
	}
}





