#include "net_hub.h"
#include <net/support/macgen.h>
#include <kernel/memory.h>
#include <kernel/verbose.h>
#include <kernel/panic.h>

#define _verbose(level, ...) verbose(level, "net_hub", __VA_ARGS__)

static bool _initialize(net_adapter_t *adapter, unsigned phy_unit, const phy_handler_t *handler);
static void _link_up(net_adapter_t *adapter);
static void _link_down(net_adapter_t *adapter);
static net_buffer_t *_get_input_buffer(net_adapter_t *adapter);
static bool _send_output_buffer(net_adapter_t *adapter, net_buffer_t *buf);
const net_driver_t __net_hub_driver = { .Initialize = _initialize,
	.LinkUp = _link_up, .LinkDown = _link_down,
	.GetInputBuffer = _get_input_buffer, .SendOutputBuffer = _send_output_buffer }; 


// ethernet hub adapter -----------------

static bool _initialize(net_adapter_t *adapter, unsigned phy_unit, const phy_handler_t *handler)
{
//	_initialize_all();

	adapter->Name = "hub";
	adapter->MaxFrameSize = ETH_MAX_FRAME_SIZE;
	adapter->Speed = 1000U;
	macgen_generate(&adapter->MAC, MAC_OUI_LOCAL, 0);

	hub_adapter_data_t *dd = (hub_adapter_data_t *)exos_mem_alloc(sizeof(hub_adapter_data_t), EXOS_MEMF_CLEAR);
	ASSERT(dd != NULL, KERNEL_ERROR_NOT_ENOUGH_MEMORY);
	adapter->DriverData = dd;

	exos_fifo_create(&dd->InputFifo, &adapter->InputEvent);
	// NOTE: output fifo is null and has to be filled later
	return true;
}

static void _link_up(net_adapter_t *adapter)
{
	_verbose(VERBOSE_DEBUG, "link_up() ignored");
}

static void _link_down(net_adapter_t *adapter)
{
	kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
}

static void _flush(net_adapter_t *adapter)
{
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);

	exos_mutex_lock(&adapter->OutputLock);
#if 0
	ncm_device_context_t *ncm_dev = (ncm_device_context_t *)adapter->DriverData;
	if (ncm_dev != NULL && ncm_dev->Ready)	// NOTE: usb function is running
	{
		unsigned done = 0;
		net_buffer_t *buf;
		while(buf = (net_buffer_t *)exos_fifo_dequeue(&ncm_dev->TxFifo), buf != NULL)
		{
			net_adapter_free_buffer(buf);
			done++;
		}
		_verbose(VERBOSE_DEBUG, "flushed %d tx buffers", done);
	}
#endif
	exos_mutex_unlock(&adapter->OutputLock);
}

static net_buffer_t *_get_input_buffer(net_adapter_t *adapter)
{
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);
	hub_adapter_data_t *dd = (hub_adapter_data_t *)adapter->DriverData;
	ASSERT(dd != NULL, KERNEL_ERROR_NULL_POINTER);

	net_buffer_t *buf = (net_buffer_t *)exos_fifo_dequeue(&dd->InputFifo);
	return buf;
}

static bool _send_output_buffer(net_adapter_t *adapter, net_buffer_t *buf)
{
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);
	hub_adapter_data_t *dd = (hub_adapter_data_t *)adapter->DriverData;
	ASSERT(dd != NULL, KERNEL_ERROR_NULL_POINTER);

	ASSERT(dd->OutputFifo != NULL, KERNEL_ERROR_NULL_POINTER);
	exos_fifo_queue(dd->OutputFifo, &buf->Node);
	return true;
}





bool net_hub_create(net_hub_t *hub)
{
	net_adapter_create(&hub->HubAdapter[0], &__net_hub_driver, 0, NULL);
	net_adapter_create(&hub->HubAdapter[1], &__net_hub_driver, 1, NULL);

	hub_adapter_data_t *dd0 = (hub_adapter_data_t *)hub->HubAdapter[0].DriverData;
	ASSERT(dd0 != NULL && dd0->OutputFifo == NULL, KERNEL_ERROR_KERNEL_PANIC);
	hub_adapter_data_t *dd1 = (hub_adapter_data_t *)hub->HubAdapter[1].DriverData;
	ASSERT(dd1 != NULL && dd1->OutputFifo == NULL, KERNEL_ERROR_KERNEL_PANIC);
	dd0->OutputFifo = &dd1->InputFifo;
	dd1->OutputFifo = &dd0->InputFifo;
	return true;
}



