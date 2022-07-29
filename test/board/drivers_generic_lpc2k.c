#include <net/board.h>
#include <kernel/machine/lpc2k/net_lpc2k.h>

static NET_ADAPTER _internal_eth_adapter = { .Driver = &__net_driver_lpc2k };

// NOTE: If you're looking for your board comm devices initialization callback here, note that Exos no longer 
// uses callbacks to add comm devices at boot up. You need to add init.c (at /support/services) to enable 
// auto-registration of your platform comm driver.

NET_ADAPTER *net_board_get_adapter(int index)
{
	switch(index)
	{
		case 0:	return &_internal_eth_adapter;
	}
	return NULL;
}

