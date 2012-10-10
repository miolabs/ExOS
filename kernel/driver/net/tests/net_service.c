// IP Stack. User Customizable Stack Integration
// by Miguel Fides
//
// Kernel integration part of the KIO System

#include <kernel/threads.h>
#include <kernel/events.h>
#include <kernel/user.h>
#include "net/user.h"
#include "net/dhcp.h"

#define THREAD_STACK_SIZE 1000
static byte _stack[THREAD_STACK_SIZE];
static KTHREAD _net_background_thread;
static void _background();

static ETH_DRIVER *_driver;

static KEVENT _ready_event;

void net_user_initialize()
{
	net_initialize();

	event_initialize(&_ready_event);
	threads_create(&_net_background_thread, _background, -1, _stack, THREAD_STACK_SIZE);
}

ETH_DRIVER *net_user_wait_ready()
{
	event_wait(&_ready_event);
	return _driver;
}

static void _background()
{
	// background driver intialization
	_driver = net_user_driver_register();
	event_set(&_ready_event);

	if (_driver != NULL)
	{
		sleep(100);

		while(1)
		{
			sleep(100);

			// call iterators at 10 Hz
#ifdef NET_ENABLE_DHCP
			net_dhcp_iterate(_driver);
#endif
		}
	}
}


