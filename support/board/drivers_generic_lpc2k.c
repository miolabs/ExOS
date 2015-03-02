#include <net/board.h>
//#include <comm/board.h>
#include <kernel/machine/lpc2k/net_lpc2k.h>
#include <kernel/machine/lpc2k/comm_lpc2k.h>

static NET_ADAPTER _internal_eth_adapter = { .Driver = &__net_driver_lpc2k };

//static COMM_DEVICE _internal_comm_device = { .Driver = &__comm_driver_lpc2k, .PortCount = 4 };
//static EXOS_TREE_DEVICE _comm_devices[] = {
//	{ .Name = "comm0", .Device = &_internal_comm_device, .Unit = 0 },
//	{ .Name = "comm1", .Device = &_internal_comm_device, .Unit = 1 },
//	{ .Name = "comm2", .Device = &_internal_comm_device, .Unit = 2 },
//	{ .Name = "comm3", .Device = &_internal_comm_device, .Unit = 3 } };

NET_ADAPTER *net_board_get_adapter(int index)
{
	switch(index)
	{
		case 0:	return &_internal_eth_adapter;
	}
	return NULL;
}

//EXOS_TREE_DEVICE *comm_board_get_device(int index)
//{
//	switch(index)
//	{
//		case 0:	return &_comm_devices[0];
//		case 1:	return &_comm_devices[1];
//		case 2:	return &_comm_devices[2];
//		case 3:	return &_comm_devices[3];
//	}
//	return NULL;
//}
