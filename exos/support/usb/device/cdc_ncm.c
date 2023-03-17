#include "cdc_ncm.h"
#include <support/usb/device_hal.h>
#include <kernel/memory.h>
#include <kernel/dispatch.h>
#include <kernel/panic.h>
#include <stdio.h>
#include <string.h>
#include <kernel/verbose.h>

#ifdef NCM_DEBUG
#define _verbose(level, ...) verbose(level, "cdc-ncm", __VA_ARGS__)
#else
#define _verbose(level, ...) { /* nothing */ }
#endif

// (virtual) ethernet adapter driver
static bool _initialize(net_adapter_t *adapter, unsigned phy_unit, const phy_handler_t *handler);
static void _link_up(net_adapter_t *adapter);
static void _link_down(net_adapter_t *adapter);
static void _flush(net_adapter_t *adapter);
static net_buffer_t *_get_input_buffer(net_adapter_t *adapter);
static bool _send_output_buffer(net_adapter_t *adapter, net_buffer_t *buf);
const net_driver_t __usb_cdc_ncm_eth_driver = { .Initialize = _initialize,
	.LinkUp = _link_up, .LinkDown = _link_down,
	.Flush = _flush,
	.GetInputBuffer = _get_input_buffer, .SendOutputBuffer = _send_output_buffer }; 

#define MAX_FRAME_SIZE 1514 // not including CRC

static void _notify_dispatch(dispatcher_context_t *context, dispatcher_t *dispatcher);
static void _tx_dispatch(dispatcher_context_t *context, dispatcher_t *dispatcher);
static void _rx_dispatch(dispatcher_context_t *context, dispatcher_t *dispatcher);

static bool _init1(usb_device_interface_t *iface, const void *instance_data);
static unsigned _fill_ia_desc(usb_device_interface_t *iface, usb_interface_association_descriptor_t *iad, unsigned buffer_size);
static unsigned _fill_if_desc(usb_device_interface_t *iface, usb_interface_descriptor_t *if_desc, unsigned buffer_size);
static unsigned _fill_class_desc(usb_device_interface_t *iface, usb_descriptor_header_t *class_desc, unsigned buffer_size);
static unsigned _fill_ep_desc(usb_device_interface_t *iface, unsigned ep_index, usb_endpoint_descriptor_t *ep_desc, unsigned buffer_size);
static bool _start(usb_device_interface_t *iface, unsigned char alternate_setting, dispatcher_context_t *context);
static void _stop(usb_device_interface_t *iface);
static bool _class_request(usb_device_interface_t *iface, usb_request_t *req, void **pdata, unsigned *plength);

static const usb_device_interface_driver_t _driver = { .Initialize = _init1,
	.FillInterfaceAssociationDescriptor = _fill_ia_desc,
	.FillInterfaceDescriptor = _fill_if_desc, 
	.FillClassDescriptor = _fill_class_desc,
	.FillEndpointDescriptor = _fill_ep_desc,
	.Start = _start, .Stop = _stop, 
	.ClassRequest = _class_request };
const usb_device_interface_driver_t *__usb_cdc_ncm_device_driver = &_driver;

static bool _init2(usb_device_interface_t *iface, const void *instance_data);
static unsigned _fill_if_desc2(usb_device_interface_t *iface, usb_interface_descriptor_t *if_desc, unsigned buffer_size);
static unsigned _fill_ep_desc2(usb_device_interface_t *iface, unsigned ep_index, usb_endpoint_descriptor_t *ep_desc, unsigned buffer_size);
static bool _start2(usb_device_interface_t *iface, unsigned char alternate_setting, dispatcher_context_t *context);
static void _stop2(usb_device_interface_t *iface);
static bool _set_if2(usb_device_interface_t *iface, unsigned short alternate_setting);
static const usb_device_interface_driver_t _driver2 = { .Initialize = _init2,
	.FillInterfaceDescriptor = _fill_if_desc2,
	.FillEndpointDescriptor = _fill_ep_desc2,
	.Start = _start2, .Stop = _stop2,
	.SetInterface = _set_if2 };

// instance tracking ---------------

static mutex_t _mutex;
static list_t _instance_list;
static unsigned _device_count = 0;

static void _initialize_all()
{
	static bool _initialized = false;

	if (!_initialized)
	{
		exos_mutex_create(&_mutex);
		list_initialize(&_instance_list);
		_initialized = true;
	}
}


// ethernet adapter -----------------

static bool _initialize(net_adapter_t *adapter, unsigned phy_unit, const phy_handler_t *handler)
{
	_initialize_all();

	adapter->MaxFrameSize = MAX_FRAME_SIZE;
	return true;
}

static void _link_up(net_adapter_t *adapter)
{
	kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
}

static void _link_down(net_adapter_t *adapter)
{
	kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
}

static void _flush(net_adapter_t *adapter)
{
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);

	exos_mutex_lock(&adapter->OutputLock);

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

	exos_mutex_unlock(&adapter->OutputLock);
}

static net_buffer_t *_get_input_buffer(net_adapter_t *adapter)
{
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);
	ncm_device_context_t *ncm_dev = (ncm_device_context_t *)adapter->DriverData;
	ASSERT(ncm_dev != NULL, KERNEL_ERROR_NULL_POINTER);
	
	unsigned short flags;
	unsigned length = net_packet_buffer_peek(&ncm_dev->RxPackets, &flags);
	if (length != 0)
	{
		net_buffer_t *buf = net_adapter_alloc_buffer(adapter);
		if (buf != NULL)
		{
			unsigned length = net_packet_buffer_pop(&ncm_dev->RxPackets, buf->Root.Buffer, buf->Root.Length);
			ASSERT(length != 0, KERNEL_ERROR_KERNEL_PANIC);
			buf->Root.Offset = 0;
			buf->Root.Length = length;
			return buf;
		}
		else
		{
			_verbose(VERBOSE_ERROR, "dropped input packet (couldn't allocate buffer)");
		}
	}
	return NULL;
}

static bool _send_output_buffer(net_adapter_t *adapter, net_buffer_t *buf)
{
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);

	bool done = false;
	exos_mutex_lock(&adapter->OutputLock);

	ncm_device_context_t *ncm_dev = (ncm_device_context_t *)adapter->DriverData;
	if (ncm_dev != NULL && ncm_dev->Ready)	// NOTE: usb function is running
	{
		exos_fifo_queue(&ncm_dev->TxFifo, &buf->Node);
		done = true;

		if (ncm_dev->Idle)
			exos_event_set(&ncm_dev->TxEvent);	// NOTE: wake up tx dispatcher
	}

	exos_mutex_unlock(&adapter->OutputLock);
	return done;
}


// usb driver -------------------------

#ifndef NCM_BULK_EP
#define NCM_BULK_EP 2	// FIXME: allocate
#endif
#ifndef NCM_INT_EP
#define NCM_INT_EP 3	// FIXME: allocate
#endif

static bool _init1(usb_device_interface_t *iface, const void *instance_data)
{
	net_adapter_t *adapter = (net_adapter_t *)instance_data;
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);

	usb_device_config_add_string(&iface->Name, "CDC-NCM Comm Interface");

	_initialize_all();

	ncm_device_context_t *ncm_dev = (ncm_device_context_t *)exos_mem_alloc(sizeof(ncm_device_context_t), EXOS_MEMF_CLEAR);
	if (ncm_dev != nullptr)
	{
		// NOTE: only chars 0-9 and A-F (upper case) (ECM Class 1.2, p. 5.4, iMACAddress)
		strcpy(ncm_dev->EthernetMacString, "1EA4AE41996F");	// FIXME: use actual hw address
		usb_device_config_add_string(&ncm_dev->EthernetMac, ncm_dev->EthernetMacString);

		ncm_dev->Adapter = adapter;
		ncm_dev->Interface = iface;
		iface->DriverContext = ncm_dev;

		net_packet_buffer_initialize(&ncm_dev->RxPackets, ncm_dev->RxBuffer, sizeof(ncm_dev->RxBuffer));
		exos_fifo_create(&ncm_dev->TxFifo, NULL);

		ncm_dev->Unit = _device_count++;

		usb_device_interface_create(&ncm_dev->SecondaryDataInterface, &_driver2);
		usb_device_config_add_interface(iface->Configuration, &ncm_dev->SecondaryDataInterface, ncm_dev); 

		adapter->DriverData = ncm_dev;
		return true;
	}
	else _verbose(VERBOSE_ERROR, "not enough memory!");

	return false;
}

static unsigned _fill_ia_desc(usb_device_interface_t *iface, usb_interface_association_descriptor_t *iad, unsigned buffer_size)
{
	ASSERT(buffer_size >= sizeof(usb_interface_association_descriptor_t), KERNEL_ERROR_KERNEL_PANIC);
	iad->Header = (usb_descriptor_header_t) { .Length = sizeof(usb_interface_association_descriptor_t),
		.DescriptorType = USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION };
	iad->InterfaceCount = 2;
	iad->FirstInterface = iface->Index;
	iad->FunctionClass = USB_CLASS_COMM;
	iad->FunctionSubclass = USB_CDC_SUBCLASS_NCM;
	iad->FunctionProtocol = USB_CDC_PROTOCOL_NONE;
	iad->FunctionNameIndex = iface->Name.Index;
	return iad->Header.Length;
}

static unsigned _fill_if_desc(usb_device_interface_t *iface, usb_interface_descriptor_t *if_desc, unsigned buffer_size)
{
	ASSERT(buffer_size >= sizeof(usb_interface_descriptor_t), KERNEL_ERROR_KERNEL_PANIC);

	if_desc->InterfaceClass = USB_CLASS_COMM;
	if_desc->InterfaceSubClass = USB_CDC_SUBCLASS_NCM;	// 0x0D = Network Control Model
	if_desc->Protocol = USB_CDC_PROTOCOL_NONE;			// no encapsulated cmds/responses
	if_desc->InterfaceNameIndex = iface->Name.Index;
#ifdef NCM_INT_EP
	if_desc->NumEndpoints = 1;
#else
	if_desc->NumEndpoints = 0;
#endif
	return sizeof(usb_interface_descriptor_t);
}

static unsigned _fill_class_desc(usb_device_interface_t *iface, usb_descriptor_header_t *class_desc, unsigned buffer_size)
{
	ncm_device_context_t *ncm_dev = (ncm_device_context_t *)iface->DriverContext;
	ASSERT(ncm_dev != NULL, KERNEL_ERROR_NULL_POINTER);

	void *ptr = (void *)class_desc;
	unsigned size = 0;

	usb_cdc_func_header_descriptor_t *fhdr = (usb_cdc_func_header_descriptor_t *)ptr;
	fhdr->Header = (usb_descriptor_header_t) { .Length = sizeof(usb_cdc_func_header_descriptor_t), 
		.DescriptorType = USB_REQTYPE_CLASS | USB_DESCRIPTOR_TYPE_INTERFACE };
	fhdr->DescriptorSubtype = USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_HEADER;
	fhdr->CDC = HTOUSB16(0x0120);	// CDC 1.2
	size += fhdr->Header.Length;

	usb_cdc_union_functional_descriptor_t *ufd = (usb_cdc_union_functional_descriptor_t *)(ptr + size);
	ufd->Header = (usb_descriptor_header_t) { .Length = sizeof(usb_cdc_union_functional_descriptor_t) + 1,
		.DescriptorType = USB_REQTYPE_CLASS | USB_DESCRIPTOR_TYPE_INTERFACE };
	ufd->DescriptorSubtype = USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_UFD;
	ufd->ControlInterface = iface->Index;
	ufd->SubordinateInterface[0] = ncm_dev->SecondaryDataInterface.Index;
	size += ufd->Header.Length;

	usb_cdc_ethernet_functional_descriptor_t *enf = (usb_cdc_ethernet_functional_descriptor_t *)(ptr + size);
	enf->Header = (usb_descriptor_header_t) { .Length = sizeof(usb_cdc_ethernet_functional_descriptor_t),
		.DescriptorType = USB_REQTYPE_CLASS | USB_DESCRIPTOR_TYPE_INTERFACE };
	enf->DescriptorSubtype = USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_ENFD;
	enf->iMACAddress = ncm_dev->EthernetMac.Index;
	enf->bmEthernetStatistics = HTOUSB32(0);	// FIXME
	enf->wMaxSegmentSize = HTOUSB16(MAX_FRAME_SIZE);
	enf->wNumberMCFilters = HTOUSB16(0);		// FIXME
	enf->bNumberPowerFilters = 0;
	size += enf->Header.Length;

	usb_cdc_ncm_functional_descriptor_t *ncm = (usb_cdc_ncm_functional_descriptor_t *)(ptr + size);
	ncm->Header = (usb_descriptor_header_t) { .Length = sizeof(usb_cdc_ncm_functional_descriptor_t),
		.DescriptorType = USB_REQTYPE_CLASS | USB_DESCRIPTOR_TYPE_INTERFACE };
	ncm->DescriptorSubtype = USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_NCM;
	ncm->bcdNcmVersion = HTOUSB16(0x0100);		// FIXME
	ncm->bmNetworkCapabilities = USB_CDC_NCM_NETCAPSF_SetGetCrcMode | USB_CDC_NCM_NETCAPSF_SetGetNetAddress;
	size += ncm->Header.Length;

	_verbose(VERBOSE_DEBUG, "class descriptor filled (%d bytes)", size);
	return size;
}

static unsigned _fill_ep_desc(usb_device_interface_t *iface, unsigned ep_index, usb_endpoint_descriptor_t *ep_desc, unsigned buffer_size)
{
	ASSERT(buffer_size >= sizeof(usb_endpoint_descriptor_t), KERNEL_ERROR_KERNEL_PANIC);

	switch(ep_index)
	{
#ifdef NCM_INT_EP
		case 0:	// notify (in) ep
			ep_desc->Address = 0x80 | NCM_INT_EP; 
			ep_desc->Attributes = USB_TT_INTERRUPT;
			ep_desc->MaxPacketSize = HTOUSB16(NCM_MAX_PACKET_LENGTH);
			ep_desc->Interval = 0;
			return sizeof(usb_endpoint_descriptor_t);
#endif
	}
	return 0;
}

static bool _start(usb_device_interface_t *iface, unsigned char alternate_setting, dispatcher_context_t *context)
{
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);
	ncm_device_context_t *ncm_dev = (ncm_device_context_t *)iface->DriverContext;

	// NOTE: alternate setting is ignored

//	exos_mutex_lock(&ncm_dev->Lock);

	ASSERT(!list_find_node(&_instance_list, &ncm_dev->Node), KERNEL_ERROR_KERNEL_PANIC);
	list_add_tail(&_instance_list, &ncm_dev->Node);
	ncm_dev->DispatcherContext = context;

	exos_event_create(&ncm_dev->NotifyEvent, EXOS_EVENTF_AUTORESET);
	ncm_dev->NotifyIo = (usb_io_buffer_t) { .Event = &ncm_dev->NotifyEvent };
	exos_dispatcher_create(&ncm_dev->NotifyDispatcher, &ncm_dev->NotifyEvent, _notify_dispatch, ncm_dev);
	exos_dispatcher_add(ncm_dev->DispatcherContext, &ncm_dev->NotifyDispatcher, 1000);	// NOTE: initial state notification
	ncm_dev->NotifyState = (ncm_notify_state_t) { /* all-zero */ };

	ncm_dev->Connected = false;

	#ifdef DEBUG
	// NOTE: fake!
	ncm_dev->Connected = true;
	ncm_dev->SpeedMbps = 100U;
	#endif

	_verbose(VERBOSE_COMMENT, "[%d] USB start comm if -------", iface->Index);

	ncm_dev->Ready = true;
	return true;
}

static void _stop(usb_device_interface_t *iface)
{
	ncm_device_context_t *ncm_dev = (ncm_device_context_t *)iface->DriverContext;
	ASSERT(ncm_dev != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(ncm_dev->Interface == iface, KERNEL_ERROR_NULL_POINTER);

	ASSERT(list_find_node(&_instance_list, &ncm_dev->Node), KERNEL_ERROR_KERNEL_PANIC);
	list_remove(&ncm_dev->Node);

	ncm_dev->Ready = false;
	exos_dispatcher_remove(ncm_dev->DispatcherContext, &ncm_dev->NotifyDispatcher);

	_verbose(VERBOSE_COMMENT, "[%d] USB stop comm if -------", iface->Index);
}

static bool _init2(usb_device_interface_t *iface, const void *instance_data)
{
	usb_device_config_add_string(&iface->Name, "CDC-NCM Data Interface");
	ASSERT(instance_data != nullptr, KERNEL_ERROR_NULL_POINTER);
	iface->DriverContext = (void *)instance_data;
	iface->AlternateSettings = 1;
}

static unsigned _fill_if_desc2(usb_device_interface_t *iface, usb_interface_descriptor_t *if_desc, unsigned buffer_size)
{
	ASSERT(buffer_size >= sizeof(usb_interface_descriptor_t), KERNEL_ERROR_KERNEL_PANIC);

	if_desc->InterfaceClass = USB_CLASS_DATA;	// CDC Data interface
	if_desc->InterfaceSubClass = 0;
	if_desc->Protocol = 1;						// 1 = Network Transfer Block
	if_desc->InterfaceNameIndex = iface->Name.Index;

	switch(if_desc->AlternateSetting)
	{
		case 0:	if_desc->NumEndpoints = 0;	break;
		case 1:	if_desc->NumEndpoints = 2;	break;
		default:	
			kernel_panic(KERNEL_ERROR_KERNEL_PANIC);
	}
	return sizeof(usb_interface_descriptor_t);
}

static unsigned _fill_ep_desc2(usb_device_interface_t *iface, unsigned ep_index, usb_endpoint_descriptor_t *ep_desc, unsigned buffer_size)
{
	ASSERT(buffer_size >= sizeof(usb_endpoint_descriptor_t), KERNEL_ERROR_KERNEL_PANIC);

	switch(ep_index)
	{
		case 0:	// tx (in) ep
			ep_desc->Address = 0x80 | NCM_BULK_EP; 
			ep_desc->Attributes = USB_TT_BULK;
			ep_desc->MaxPacketSize = HTOUSB16(NCM_MAX_PACKET_LENGTH);
			ep_desc->Interval = 0;
			return sizeof(usb_endpoint_descriptor_t);
		case 1:	// rx (out) ep
			ep_desc->Address = NCM_BULK_EP;
			ep_desc->Attributes = USB_TT_BULK;
			ep_desc->MaxPacketSize = HTOUSB16(NCM_MAX_PACKET_LENGTH);
			ep_desc->Interval = 0;
			return sizeof(usb_endpoint_descriptor_t);
	}
	return 0;
}

static bool _enable_data_if(usb_device_interface_t *iface, bool enable)
{
	ncm_device_context_t *ncm_dev = (ncm_device_context_t *)iface->DriverContext;

	if (!enable)
	{
		// NOTE: alt=0 should cancel all activity AND reset settings (crc_mode, hw_address, etc.)

		// remove dispatchers
		exos_dispatcher_remove(ncm_dev->DispatcherContext, &ncm_dev->RxDispatcher);
		exos_dispatcher_remove(ncm_dev->DispatcherContext, &ncm_dev->TxDispatcher);

		_verbose(VERBOSE_COMMENT, "[%d] data if disabled/reset!", iface->Index);
	}
	else
	{
		// NOTE: in/out ep enabled, enable activity now

		exos_event_create(&ncm_dev->RxEvent, EXOS_EVENTF_AUTORESET);
		exos_dispatcher_create(&ncm_dev->RxDispatcher, &ncm_dev->RxEvent, _rx_dispatch, ncm_dev);
		exos_dispatcher_add(ncm_dev->DispatcherContext, &ncm_dev->RxDispatcher, EXOS_TIMEOUT_NEVER);
		ncm_dev->RxIo = (usb_io_buffer_t) { .Event = &ncm_dev->RxEvent,
			.Data = ncm_dev->RxData, .Length = sizeof(ncm_dev->RxData) };
		usb_set_rx_buffer(NCM_BULK_EP, &ncm_dev->RxIo);

		exos_event_create(&ncm_dev->TxEvent, EXOS_EVENTF_AUTORESET);
		ncm_dev->TxIo = (usb_io_buffer_t) { .Event = &ncm_dev->TxEvent };
		exos_dispatcher_create(&ncm_dev->TxDispatcher, &ncm_dev->TxEvent, _tx_dispatch, ncm_dev);
		exos_dispatcher_add(ncm_dev->DispatcherContext, &ncm_dev->TxDispatcher, EXOS_TIMEOUT_NEVER);
		ncm_dev->Idle = true;

		_verbose(VERBOSE_COMMENT, "[%d] data if enabled!", iface->Index);
	}
}

static bool _start2(usb_device_interface_t *iface, unsigned char alternate_setting, dispatcher_context_t *context)
{
	ncm_device_context_t *ncm_dev = (ncm_device_context_t *)iface->DriverContext;
	ASSERT(list_find_node(&_instance_list, &ncm_dev->Node), KERNEL_ERROR_KERNEL_PANIC);
	// NOTE: both interface should run in the same context
	ASSERT(ncm_dev->DispatcherContext == context, KERNEL_ERROR_KERNEL_PANIC);

	// NOTE: usb device stack have enabled/disabled our endpoints before calling us...
	_verbose(VERBOSE_COMMENT, "[%d] USB start data if ------- (alt=%d)", iface->Index, alternate_setting);
	_enable_data_if(iface, alternate_setting != 0);
	return true;
}

static bool _set_if2(usb_device_interface_t *iface, unsigned short alternate_setting)
{
	ncm_device_context_t *ncm_dev = (ncm_device_context_t *)iface->DriverContext;
	ASSERT(list_find_node(&_instance_list, &ncm_dev->Node), KERNEL_ERROR_KERNEL_PANIC);
	_verbose(VERBOSE_COMMENT, "[%d] USB set_interface ------- (alt=%d)", iface->Index, alternate_setting);
	_enable_data_if(iface, alternate_setting != 0);
	return true;
}

static void _stop2(usb_device_interface_t *iface)
{
	ncm_device_context_t *ncm_dev = (ncm_device_context_t *)iface->DriverContext;
	ASSERT(ncm_dev != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(&ncm_dev->SecondaryDataInterface == iface, KERNEL_ERROR_NULL_POINTER);

	_enable_data_if(iface, false);

	_verbose(VERBOSE_COMMENT, "[%d] USB stop data if -------", iface->Index);
}


static void _notify_dispatch(dispatcher_context_t *context, dispatcher_t *dispatcher)
{
	ncm_device_context_t *ncm_dev = (ncm_device_context_t *)dispatcher->CallbackState;
	ASSERT(ncm_dev != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(list_find_node(&_instance_list, &ncm_dev->Node), KERNEL_ERROR_KERNEL_PANIC);

	usb_io_status_t sta = ncm_dev->NotifyIo.Status;
	if (sta == USB_IOSTA_DONE || sta == USB_IOSTA_UNKNOWN)
	{
		unsigned size = 0;
		usb_cdc_notify_header_t *hdr = (usb_cdc_notify_header_t *)ncm_dev->NotifyData;
		*hdr = (usb_cdc_notify_header_t) { .RequestType = USB_REQTYPE_DEVICE_TO_HOST | USB_REQTYPE_CLASS | USB_REQTYPE_RECIPIENT_INTERFACE,
			.Index = ncm_dev->Interface->Index };
		if (ncm_dev->NotifyState.Connected != ncm_dev->Connected)
		{
			ncm_dev->NotifyState.Connected = ncm_dev->Connected;

			hdr->RequestCode = USB_CDC_NOTIFY_NETWORK_CONNECTION; 
			hdr->Value = ncm_dev->Connected ? 0x1 : 0x0;
			size = sizeof(usb_cdc_notify_header_t);
		}
		else if (ncm_dev->NotifyState.SpeedMbps != ncm_dev->SpeedMbps)
		{
			ncm_dev->NotifyState.SpeedMbps = ncm_dev->SpeedMbps;

			hdr->RequestCode = USB_CDC_NOTIFY_CONNECTION_SPEED_CHANGE;
			hdr->Length = sizeof(usb_cdc_notify_speed_t);
			size = sizeof(usb_cdc_notify_header_t);

			usb_cdc_notify_speed_t *spd = (usb_cdc_notify_speed_t *)(((void *)hdr) + size);
			spd->DSBitRate = spd->USBitRate = HTOUSB32(ncm_dev->SpeedMbps * 1000000U);
			size += sizeof(usb_cdc_notify_speed_t);
		}

		if (size != 0 && ncm_dev->Ready)
		{
			ncm_dev->NotifyIo.Data = ncm_dev->NotifyData;
			ncm_dev->NotifyIo.Length = size;
			usb_set_tx_buffer(NCM_INT_EP, &ncm_dev->NotifyIo);

			_verbose(VERBOSE_DEBUG, "[%d] notify (%d bytes)", ncm_dev->Interface->Index, size);
		}
		else
		{
			_verbose(VERBOSE_DEBUG, "[%d] notify idle (%s)", ncm_dev->Interface->Index,
				ncm_dev->Ready ? "ready" : "not ready");
		}
	}
	else if (sta == USB_IOSTA_ERROR)
	{
		_verbose(VERBOSE_DEBUG, "[%d] notify error", ncm_dev->Interface->Index);
	}
	else 
	{
		_verbose(VERBOSE_DEBUG, "[%d] notify wait...", ncm_dev->Interface->Index);
	}

	exos_dispatcher_add(context, dispatcher, EXOS_TIMEOUT_NEVER);
}

static unsigned _mbuf_count(net_mbuf_t *mbuf)
{
	unsigned count = 0;
	while(mbuf != NULL && mbuf->Buffer != NULL && mbuf->Length != 0)
	{
		count++;
		mbuf = mbuf->Next;
	}
	return count;
}

static unsigned _assemble(ncm_device_context_t *ncm_dev)
{
	net_adapter_t *adapter = ncm_dev->Adapter;
	ASSERT(adapter != NULL && adapter->DriverData == ncm_dev, KERNEL_ERROR_KERNEL_PANIC);

	exos_mutex_lock(&adapter->OutputLock);

	ncm_transfer_header16_t *nth16 = (ncm_transfer_header16_t *)ncm_dev->TxData;
	unsigned pkt_cnt = 0;
	unsigned offset = 0;
	usb16_t *prev_ndp_next_index = NULL;
	net_buffer_t *buf;
	while(buf = (net_buffer_t *)exos_fifo_dequeue(&ncm_dev->TxFifo), buf != NULL)
	{
		pkt_cnt++;

		if (offset == 0)
		{
			offset = sizeof(ncm_transfer_header16_t);
			ASSERT((offset & 3) == 0, KERNEL_ERROR_KERNEL_PANIC);

			nth16->dwSignature = HTOUSB32(SIG_NTH16);
			nth16->wHeaderLength = HTOUSB16(sizeof(ncm_transfer_header16_t));
			nth16->wSequence = HTOUSB16(ncm_dev->TxSequence);
			// NOTE: wBlockLength is set later
			nth16->wNdpIndex = HTOUSB16(offset >> 2);
			ncm_dev->TxSequence++;
		}

		net_mbuf_t *mbuf = &buf->Root;
		unsigned seg_cnt = _mbuf_count(mbuf);
		ASSERT(seg_cnt != 0, KERNEL_ERROR_KERNEL_PANIC);
		unsigned seg_done = 0;

		ncm_datagram_pointer16_t *ndp16 = (ncm_datagram_pointer16_t *)((unsigned char *)nth16 + offset);
		unsigned ndp_length = sizeof(ncm_datagram_pointer16_t) + ((seg_cnt + 1) * sizeof(struct ncm_datagram16));
		offset += ndp_length;
		ASSERT((offset & 3) == 0, KERNEL_ERROR_KERNEL_PANIC);
		if (offset < sizeof(ncm_dev->TxData)) 
		{
			if (prev_ndp_next_index != NULL) *prev_ndp_next_index = HTOUSB16(offset >> 2);
			ndp16->dwSignature = HTOUSB32(SIG_NDP16);
			ndp16->wLength = HTOUSB16(ndp_length);
			ndp16->wNextNdpIndex = HTOUSB16(0);	// NOTE: re-written later if needed
			prev_ndp_next_index = &ndp16->wNextNdpIndex;
			do
			{
				ndp16->array[seg_done] = (struct ncm_datagram16) { 
					.wDatagramIndex = HTOUSB16(offset >> 2), 
					.wDatagramLength =HTOUSB16(mbuf->Length) };

				unsigned word_count = (mbuf->Length + 3) >> 2;
				if ((offset + (word_count << 2)) > sizeof(ncm_dev->TxData))
					break;

				memcpy((unsigned char *)nth16 + offset, mbuf->Buffer + mbuf->Offset, mbuf->Length);
				offset += word_count << 2;
				mbuf = mbuf->Next;
				seg_done++;
			} while (mbuf != NULL && mbuf->Buffer != NULL && mbuf->Length != 0);
		
			ndp16->array[seg_done] = (struct ncm_datagram16) { .wDatagramLength = 0 };
		}

		net_adapter_free_buffer(buf);
		_verbose(VERBOSE_DEBUG, "asm freed buffer %d ($%x)", pkt_cnt, (unsigned)buf & 0xffff);

		if (seg_cnt != seg_done)
		{
			_verbose(VERBOSE_ERROR, "output packet truncated!");
			break;
		}
	}
	nth16->wBlockLength = HTOUSB16(offset);
	exos_mutex_unlock(&adapter->OutputLock);

	_verbose(VERBOSE_DEBUG, "assembled %d packets", pkt_cnt);
	return offset;
}

static void _tx_dispatch(dispatcher_context_t *context, dispatcher_t *dispatcher)
{
	ncm_device_context_t *ncm_dev = (ncm_device_context_t *)dispatcher->CallbackState;

	usb_io_status_t sta = ncm_dev->TxIo.Status;
	if (sta == USB_IOSTA_IN_WAIT || sta == USB_IOSTA_IN_COMPLETE)
	{
		if (ncm_dev->Ready)
		{
			ASSERT(ncm_dev->Interface != nullptr, KERNEL_ERROR_NULL_POINTER);
			_verbose(VERBOSE_DEBUG, "[%d] busy", ncm_dev->Interface->Index);

			// continue waiting
			ncm_dev->Idle = false;
			exos_dispatcher_add(context, &ncm_dev->TxDispatcher, 1000);
		}
		else
		{
			ASSERT(ncm_dev->Interface != NULL, KERNEL_ERROR_NULL_POINTER);
			_verbose(VERBOSE_ERROR, "[%d] timeout error! (stop)", ncm_dev->Interface->Index);
			ncm_dev->TxIo.Status = USB_IOSTA_UNKNOWN;
		}
	}
	else if (sta == USB_IOSTA_ERROR)
	{
		ASSERT(ncm_dev->Interface != NULL, KERNEL_ERROR_NULL_POINTER);
		_verbose(VERBOSE_ERROR, "[%d] tx error! (stop)", ncm_dev->Interface->Index);
		ncm_dev->TxIo.Status = USB_IOSTA_UNKNOWN;
	}
	else
	{
		unsigned done = _assemble(ncm_dev);

		if (done != 0)
		{
#ifdef NCM_DEBUG
			//static unsigned char vbuf[IAP2_MAX_PACKET_LENGTH + 1];
			//for(unsigned i = 0; i < done; i++)
			//{
			//	unsigned char c = iap2dev->TxData[2 + i];
			//	vbuf[i] = (c >= ' ' && c <= 'z') ? c : '.';
			//}
			//vbuf[done] = '\0';	
			//_verbose(VERBOSE_DEBUG,"[%d] tx '%s'", iap2dev->Interface->Index, vbuf);
#endif
			ncm_dev->Idle = false;
 
			ncm_dev->TxIo.Data = ncm_dev->TxData;
			ncm_dev->TxIo.Length = done;
			usb_set_tx_buffer(NCM_BULK_EP, &ncm_dev->TxIo);
		}
		else
		{
#ifdef NCM_DEBUG
			if (!ncm_dev->Idle) _verbose(VERBOSE_DEBUG, "[%d] idle", ncm_dev->Interface->Index);
#endif
			ncm_dev->Idle = true;
		}

		exos_dispatcher_add(context, &ncm_dev->TxDispatcher, ncm_dev->Idle ? EXOS_TIMEOUT_NEVER : 100);	// FIXME
	}
}

struct dissect_state {
	ncm_transfer_header16_t *nth;
	ncm_datagram_pointer16_t *ndp;
	unsigned short index;
	};

static bool _validate_ndp(ncm_transfer_header16_t *hth16, ncm_datagram_pointer16_t *ndp16)
{
	unsigned sig = USB32TOH(ndp16->dwSignature);
	unsigned short hdr_len = USB16TOH(ndp16->wLength);

	if ((sig & 0xffffff) != SIG_NDP16)
	{
		_verbose(VERBOSE_ERROR, "dissect ndp is not NDP16");
		return false;
	}
	if ((hdr_len & 3) != 0 || (hdr_len >> 2) < 2)	// must be 8+k*4 bytes
	{
		_verbose(VERBOSE_ERROR, "dissect NDP16 has wrong header length");
		return false;
	}
	return true;
}

static bool _dissect_start(struct dissect_state *state, unsigned char *data, unsigned data_length)
{
	ncm_transfer_header16_t *hdr16 = (ncm_transfer_header16_t *)data;
	unsigned sig = USB32TOH(hdr16->dwSignature);
	unsigned hdr_len = USB16TOH(hdr16->wHeaderLength);

	if (sig != SIG_NTH16)
	{
		_verbose(VERBOSE_ERROR, "dissect packet is not NTB16");
		return false;
	}
	if (hdr_len != sizeof(ncm_transfer_header16_t))	// 12 bytes
	{
		_verbose(VERBOSE_ERROR, "dissect packet NTB16 has wrong header length");
		return false;
	}

	// TODO: input Sequence is ignored
	//unsigned short seq = USB16TOH(hdr16->wSequence);
	unsigned short length = USB16TOH(hdr16->wBlockLength);
	if (length < hdr_len || length > data_length)
	{
		_verbose(VERBOSE_ERROR, "dissect packet NTB16 has wrong total length");
		return false;
	}

	unsigned short ndp_offset = USB16TOH(hdr16->wNdpIndex);
	if (ndp_offset <  hdr_len ||
		(ndp_offset + sizeof(ncm_datagram_pointer16_t)) > length)
	{
		_verbose(VERBOSE_ERROR, "dissect packet NTB16 has index out of range");
		return false;
	}

	ncm_datagram_pointer16_t *ndp = (ncm_datagram_pointer16_t *)(data + ndp_offset);
	if (_validate_ndp(hdr16, ndp))
	{
		*state = (struct dissect_state) { .nth = hdr16, .ndp = ndp, .index = 0 };
		return true;
	} 

	return false;
}

static void *_dissect(struct dissect_state *state, unsigned *plen)
{
	ASSERT(state != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(state->nth != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(state->ndp != NULL, KERNEL_ERROR_NULL_POINTER);

	unsigned short ndp_len = USB16TOH(state->ndp->wLength);
	ASSERT(ndp_len >= sizeof(ncm_datagram_pointer16_t), KERNEL_ERROR_KERNEL_PANIC); 
	unsigned count = (ndp_len - sizeof(ncm_datagram_pointer16_t)) >> 2;

	if (state->index < count)
	{
		unsigned offset = USB16TOH(state->ndp->array[state->index].wDatagramIndex);
		unsigned length = USB16TOH(state->ndp->array[state->index].wDatagramLength);
		
		state->index++;
		if (state->index == count)
		{
			unsigned index = USB16TOH(state->ndp->wNextNdpIndex);
			if (index != 0)
			{
				ncm_datagram_pointer16_t *next = (ncm_datagram_pointer16_t *)
					((unsigned char *)state->nth + index);
				if (_validate_ndp(state->nth, next))
				{
					state->ndp = next;
					state->index = 0;
				}
			}
		}

		*plen = length;
		return length != 0 ? (unsigned char *)state->nth + offset : NULL; 
	}

	return false;
}

static void _rx_dispatch(dispatcher_context_t *context, dispatcher_t *dispatcher)
{
	ncm_device_context_t *ncm_dev = (ncm_device_context_t *)dispatcher->CallbackState;
	if (ncm_dev->RxIo.Status == USB_IOSTA_DONE)
	{
		// NOTE: dissect usb buffer to find datagrams, then send them to ethernet framework
		struct dissect_state dis;
		if (_dissect_start(&dis, ncm_dev->RxIo.Data, ncm_dev->RxIo.Done))
		{
			unsigned length;
			void *data;
			while(data = _dissect(&dis, &length), data != NULL)
			{
#ifdef USB_DEVICE_DEBUG
				//_verbose(VERBOSE_DEBUG, "rx (%d bytes)", length);
#endif
				if (!net_packet_buffer_push(&ncm_dev->RxPackets, data, length, 0))
					_verbose(VERBOSE_ERROR, "failed to push rx data");
			}

			ASSERT(ncm_dev->Adapter != NULL, KERNEL_ERROR_NULL_POINTER);
			exos_event_set(&ncm_dev->Adapter->InputEvent);
		}

		// reset input ep
		ncm_dev->RxIo.Data = ncm_dev->RxData;
		ncm_dev->RxIo.Length = sizeof(ncm_dev->RxData);
		usb_set_rx_buffer(NCM_BULK_EP, &ncm_dev->RxIo);

		exos_dispatcher_add(context, &ncm_dev->RxDispatcher, EXOS_TIMEOUT_NEVER);
	}
	else
	{
		ASSERT(ncm_dev->Interface != NULL, KERNEL_ERROR_NULL_POINTER);
		_verbose(VERBOSE_COMMENT, "[%d] rx stopped! -----", ncm_dev->Interface->Index);
	}
}


static bool _eth_request(usb_device_interface_t *iface, usb_request_t *req, void **pdata, unsigned *plength);
static bool _ncm_request(usb_device_interface_t *iface, usb_request_t *req, void **pdata, unsigned *plength);

static bool _class_request(usb_device_interface_t *iface, usb_request_t *req, void **pdata, unsigned *plength)
{
	ncm_device_context_t *ncm_dev = (ncm_device_context_t *)iface->DriverContext;
	ASSERT(ncm_dev != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(ncm_dev->Interface == iface, KERNEL_ERROR_NULL_POINTER);

	// NOTE: req codes beginning from 0x40 are ethernet related
	if ((req->RequestCode & 0xf0) == 0x40)
		return _eth_request(iface, req, pdata, plength); 
	// NOTE: req codes beginning from 0x80 are NCM related
	if ((req->RequestCode & 0xf0) == 0x80)
		return _ncm_request(iface, req, pdata, plength); 

	_verbose(VERBOSE_DEBUG, "[%d] class req type=$%02x code=$%02x value=$%04x index=$%04x len=$%04x (not implemented)", iface->Index, 
		req->RequestType, req->RequestCode, req->Value, req->Index, req->Length);
	return false;
}

static bool _eth_request(usb_device_interface_t *iface, usb_request_t *req, void **pdata, unsigned *plength)
{
	_verbose(VERBOSE_DEBUG, "[%d] class ETH req type=$%02x code=$%02x value=$%04x index=$%04x len=$%04x (not implemented)", iface->Index, 
		req->RequestType, req->RequestCode, req->Value, req->Index, req->Length);
	return false;
}

static void _init_parameters(ncm_ntb_parameter_structure_t *nps)
{
	*nps = (ncm_ntb_parameter_structure_t) {
		.wLength = HTOUSB16(sizeof(ncm_ntb_parameter_structure_t)),
		.bmNtbFormatsSupported = HTOUSB16(NCM_NTB_FORMAT_SUPP_NTB16),
		.dwNtbInMaxSize = HTOUSB32(NCM_MAX_NTB_SIZE),
		.wNdpInDivisor = HTOUSB16(4),
		.wNdpInPayloadRemainder = HTOUSB16(0),
		.wNdpInAlignment = HTOUSB16(4),
		.dwNtbOutMaxSize = HTOUSB32(NCM_MAX_NTB_SIZE),
		.wNdpOutDivisor = HTOUSB16(4),
		.wNdpOutPayloadRemainder = HTOUSB16(0),
		.wNdpOutAlignment = HTOUSB16(4),
		.wNtbOutMaxDatagrams = HTOUSB16(0)
	};
}

static bool _ncm_request(usb_device_interface_t *iface, usb_request_t *req, void **pdata, unsigned *plength)
{
	static ncm_ntb_parameter_structure_t _parameters;
	static unsigned short _ret_val;

	switch(req->RequestCode)
	{
		case NCM_GET_NTB_PARAMETERS:
			_verbose(VERBOSE_DEBUG, "[%d] GET_NTB_PARAMETERS", iface->Index);
			_init_parameters(&_parameters);
			*pdata = &_parameters;
			*plength = sizeof(ncm_ntb_parameter_structure_t);
			return true;
		case NCM_GET_CRC_MODE:
			_verbose(VERBOSE_DEBUG, "[%d] GET_CRC_MODE", iface->Index);
			*pdata = &_ret_val;
			_ret_val = 0;	// TODO: use backend actual value		
			return true;
		case NCM_SET_CRC_MODE:
			_verbose(VERBOSE_DEBUG, "[%d] SET_CRC_MODE (Value=$%x)", iface->Index, req->Value);
			_ret_val = req->Value;	// TODO: configure eth backend
			return true;

		default:
			_verbose(VERBOSE_DEBUG, "[%d] class NCM req type=$%02x code=$%02x value=$%04x index=$%04x len=$%04x (not implemented)", iface->Index, 
				req->RequestType, req->RequestCode, req->Value, req->Index, req->Length);
			break;
	}
	return false;
}




