#include "canopen.h"
#include "canopen_service.h"

static int _can_module;

int canopen_initialize(int can_module, int bitrate)
{
	can_receiver_initialize();
	
	_can_module = can_module;
	int done = hal_can_initialize(can_module, bitrate, CAN_INITF_NONE);
	if (done)
	{
		canopen_service_initialize(can_module);
		return 1;
	}
	return 0;
}

int canopen_slave_create(CANOPEN_INSTANCE *ci, const CANOPEN_NODE_IDENTITY *identity)
{
	*ci = (CANOPEN_INSTANCE) { .Identity = identity };
	return 1;
}

int canopen_slave_add(CANOPEN_INSTANCE *ci)
{
	int done = 0;
//	exos_mutex_lock(&_instances_lock);
//	if (!_find_instance(ci->Identity->Node))
//	{
//		list_add_tail(&_instances, (EXOS_NODE *)ci);
//		done = 1;
//	}
//	exos_mutex_unlock(&_instances_lock);

//	if (done)
//	{
//		ci->State = CANOPEN_NODE_BOOTUP;
//		_send_node_guard(ci);
//		ci->State = CANOPEN_NODE_PRE_OPERATIONAL;
//	}

	return done;
}

int canopen_nmt_send_cmd(int cmd, int target_node)
{
	CAN_EP ep = (CAN_EP) { .Bus = _can_module, .Id = 0x000 };
	CANOPEN_NMT_MSG data = (CANOPEN_NMT_MSG) { .Cmd = cmd, .NodeId = target_node };
	int done = hal_can_send(ep, (CAN_BUFFER *)&data, sizeof(data), CANF_NONE);
	return done;
}

int canopen_master_sync()
{
	CAN_EP ep = (CAN_EP) { .Bus = _can_module, .Id = 0x080 };
	int done = hal_can_send(ep, NULL, 0, CANF_NONE);
	return done;
}



int canopen_sdo_read(int target_node, CANOPEN_MUX mux, void *data)
{
}

int canopen_sdo_read_expedited(int target_node, CANOPEN_MUX mux, unsigned long *data)
{
}





