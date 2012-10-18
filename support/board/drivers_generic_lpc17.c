#include <net/board.h>
#include <comm/board.h>
#include <kernel/machine/lpc17/net_lpc17.h>
#include <kernel/machine/lpc17/comm_lpc17.h>

static ETH_ADAPTER _internal_adapter = { .Driver = &__net_driver_lpc17 };
static COMM_DEVICE _internal_comm_device = { .Driver = &__comm_driver_lpc17, .PortCount = 4 };
static EXOS_TREE_DEVICE _comm_devices[] = {
	{ .Name = "comm0", .Device = &_internal_comm_device, .Port = 0 },
	{ .Name = "comm1", .Device = &_internal_comm_device, .Port = 1 },
	{ .Name = "comm2", .Device = &_internal_comm_device, .Port = 2 },
	{ .Name = "comm3", .Device = &_internal_comm_device, .Port = 3 } };

ETH_ADAPTER *net_board_get_adapter(int index)
{
	switch(index)
	{
		case 0:	return &_internal_adapter;
	}
	return NULL;
}

EXOS_TREE_DEVICE *comm_board_get_device(int index)
{
	switch(index)
	{
		case 0:	return &_comm_devices[0];
		case 1:	return &_comm_devices[1];
		case 2:	return &_comm_devices[2];
		case 3:	return &_comm_devices[3];
	}
	return NULL;
}
