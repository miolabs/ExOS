#include "canopen_service.h"
#include <kernel/dispatch.h>

static int _can_module;
static EXOS_THREAD _thread;
static unsigned char _stack[1024] __attribute__((aligned(8)));
static EXOS_DISPATCHER_CONTEXT _context;
static EXOS_LIST _instances;
static CAN_HANDLER _handler;

static void *_service(void *args);

void canopen_service_initialize(int can_module)
{
	list_initialize(&_instances);

	_can_module = can_module;
	exos_dispatcher_context_create(&_context);
	exos_thread_create(&_thread, 0, _stack, sizeof(_stack), NULL, _service, NULL);
}

int _send_node_guard(CANOPEN_INSTANCE *ci)
{
	CAN_EP ep = (CAN_EP) { .Bus = _can_module, .Id = 0x700 + ci->Identity->Node };
	unsigned char data = ci->State;
	if (ci->State != CANOPEN_NODE_BOOTUP)
	{
		if (ci->Flags.NodeGuardToggle) data |= 0x80;
		ci->Flags.NodeGuardToggle ^= 1;
	}
	int done = hal_can_send(ep, (CAN_BUFFER *)&data, 1, CANF_NONE);
	return done;
}

static int _send_sdo(int cob, CANOPEN_SDO_MSG *msg)
{
	CAN_EP ep = (CAN_EP) { .Bus = _can_module, .Id = cob };
	int done = hal_can_send(ep, (CAN_BUFFER *)msg, 8, CANF_NONE);
	return done;
}

static CANOPEN_INSTANCE *_find_instance(int node_id)
{
	FOREACH(node, &_instances)
	{
		CANOPEN_INSTANCE *ci = (CANOPEN_INSTANCE *)node;
		if (ci->Identity->Node == node_id)
			return ci;
	}
	return NULL;
}

static void _rx(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher)
{
	CANOPEN_INSTANCE *ci;
	CAN_MSG msg;
	while(1)
	{
		if (can_receiver_get(&_handler, &msg))
		{
			int cob_base = msg.EP.Id & 0x780; // mask lower 7 bits
			int target_node = msg.EP.Id & 0x7F;
			if (target_node != 0)
			{
				ci = _find_instance(target_node);
				if (ci == NULL) continue;

				switch(cob_base)
				{
					case 0x600:
						break;
				}
			}
			else
			{
				switch(cob_base)
				{
					case 0x600:
						// TODO
						break;
				}
			}
		}
	}
}

static void *_service(void *args)
{
	can_receiver_add_handler(&_handler, _can_module, 0, 0);

	EXOS_DISPATCHER rx;
	exos_dispatcher_create(&rx, &_handler.RxPort.Event, _rx, NULL);
	exos_dispatcher_add(&_context, &rx, 1000);

	while(1)
	{
		exos_dispatch(&_context, EXOS_TIMEOUT_NEVER);
	}
}

