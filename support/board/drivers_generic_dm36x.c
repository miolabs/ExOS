#include <support/net_hal.h>
#include <support/comm_hal.h>
#include <kernel/machine/dm36x/net_dm36x.h>
//#include <kernel/machine/dm36x/comm_dm36x.h>

static ETH_ADAPTER _internal_adapter = { .Driver = &__net_driver_dm36x };

ETH_ADAPTER *hal_net_get_adapter(int index)
{
	switch(index)
	{
		case 0:	return &_internal_adapter;
	}
	return NULL;
}

/*
static COMM_DEVICE _internal_comm_device = { .Driver = &__comm_driver_dm36x, .PortCount = 2 };
static EXOS_TREE_DEVICE _comm_devices[] = {
	{ .Name = "comm0", .Device = &_internal_comm_device, .Port = 0 },
	{ .Name = "comm1", .Device = &_internal_comm_device, .Port = 1 },
	};

EXOS_TREE_DEVICE *hal_comm_get_device(int index)
{
	switch(index)
	{
		case 0:	return &_comm_devices[0];
		case 1:	return &_comm_devices[1];
	}
	return NULL;
}
*/

