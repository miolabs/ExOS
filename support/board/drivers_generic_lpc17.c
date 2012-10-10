#include <support/net_hal.h>
#include <kernel/machine/lpc17/net_lpc17.h>
#include <kernel/machine/lpc17/comm_lpc17.h>

static ETH_ADAPTER _internal_adapter = { .Driver = &__net_driver_lpc17 };
static COMM_DEVICE _internal_comm_device = { .Driver = &__comm_driver_lpc17, .PortCount = 4 };

ETH_ADAPTER *hal_net_get_adapter(int index)
{
	switch(index)
	{
		case 0:	return &_internal_adapter;
	}
	return NULL;
}

COMM_DEVICE *hal_comm_get_device(int index)
{
	switch(index)
	{
		case 0:	return &_internal_comm_device;
	}
	return NULL;
}
