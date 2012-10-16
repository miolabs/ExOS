#include "net_service.h"
#include "net.h"
#include "adapter.h"
#include <kernel/thread.h>
#include <kernel/signal.h>

static void *_service(void *arg);

void net_service_start(ETH_ADAPTER *adapter)
{
	exos_thread_create(&adapter->Thread, 1, &adapter->Stack, NET_ADAPTER_THREAD_STACK, _service, adapter);
}

static void *_service(void *arg)
{
	ETH_ADAPTER *adapter = (ETH_ADAPTER *)arg;
	adapter->InputSignal = exos_signal_alloc();

	const ETH_DRIVER *driver = adapter->Driver;
	while(1)
	{
		if (adapter->Speed != 0)
		{
			net_adapter_input(adapter);
		}
		else
		{
			driver->LinkUp(adapter);
		}

		exos_signal_wait(1 << adapter->InputSignal, 1000);
	}
}

