#include "iap2_device.h"
#include <support/usb/device_hal.h>
#include <support/uart_hal.h>
#include <kernel/memory.h>
#include <kernel/dispatch.h>
#include <kernel/panic.h>
#include <stdio.h>
#include <kernel/verbose.h>

#ifdef IAP2_DEBUG
#define _verbose(level, ...) verbose(level, "iAP2-device", __VA_ARGS__)
#else
#define _verbose(level, ...) { /* nothing */ }
#endif

static bool _initialize(usb_device_interface_t *iface, const void *instance_data);
static unsigned _measure_if_desc(usb_device_interface_t *iface);
static unsigned _fill_if_desc(usb_device_interface_t *iface, usb_interface_descriptor_t *if_desc, unsigned buffer_size);
static unsigned _fill_ep_desc(usb_device_interface_t *iface, unsigned ep_index, usb_endpoint_descriptor_t *ep_desc, unsigned buffer_size);
static bool _start(usb_device_interface_t *iface, unsigned char alternate_setting, dispatcher_context_t *context);
static void _stop(usb_device_interface_t *iface);

static const usb_device_interface_driver_t _driver = { .Initialize = _initialize,
	.MeasureInterfaceDescriptors = _measure_if_desc, .FillInterfaceDescriptor = _fill_if_desc, 
	.FillEndpointDescriptor = _fill_ep_desc,
	.Start = _start, .Stop = _stop };
const usb_device_interface_driver_t *__usb_iap2_device_driver = &_driver;

#if 0
static io_error_t _open(io_entry_t *io, const char *path, io_flags_t flags);
static void _close(io_entry_t *io);
static int _read(io_entry_t *io, unsigned char *buffer, unsigned int length);
static int _write(io_entry_t *io, const unsigned char *buffer, unsigned int length);
static const io_driver_t _io_driver = {
	.Open = _open, .Close = _close,
	.Read = _read, .Write = _write };
#endif

static bool _iap2_send(iap2_transport_t *t, const unsigned char *data, unsigned length);
static unsigned short _iap2_identify(iap2_transport_t *t, iap2_control_parameters_t *params);
static const iap2_transport_driver_t _iap2_driver = {
	.Send = _iap2_send,
	.Identify = _iap2_identify }; 

static bool _initialized = false;
static list_t _instance_list;
static unsigned _device_count = 0;
static unsigned _fill_string(int *poffset, const char *s);

#ifndef IAP2_BULK_EP
#define IAP2_BULK_EP 2	// FIXME: allocate
#endif


static bool _initialize(usb_device_interface_t *iface, const void *instance_data)
{
	usb_device_config_add_string(&iface->Name, "iAP Interface");

	if (!_initialized)
	{
		// FIXME: removed because cannot be called twice (it was done already by iap2-hid)
		//iap2_initialize();

		list_initialize(&_instance_list);
		_initialized = true;
	}

	iap2_device_context_t *iap2dev = (iap2_device_context_t *)exos_mem_alloc(sizeof(iap2_device_context_t), EXOS_MEMF_CLEAR);
	if (iap2dev != nullptr)
	{
		//iap2dev->Latency = 16;
		//iap2dev->TxSize = 0;
		exos_event_create(&iap2dev->TxEvent, EXOS_EVENTF_AUTORESET);
		exos_event_create(&iap2dev->RxEvent, EXOS_EVENTF_AUTORESET);

		iap2dev->Interface = iface;
		iface->DriverContext = iap2dev;

		exos_mutex_create(&iap2dev->Lock);
//		iap2dev->Entry = nullptr;

		iap2dev->Unit = _device_count++;
		//sprintf(iap2dev->DeviceName, "/dev/iap%d", iap2dev->Unit);
		//exos_io_add_device(&iap2dev->DeviceNode, iap2dev->DeviceName, &_io_driver, iap2dev);

		return true;
	}
	else _verbose(VERBOSE_ERROR, "not enough memory!");

	return false;
}


static unsigned _measure_if_desc(usb_device_interface_t *iface)
{
	return sizeof(usb_interface_descriptor_t) + (2 * sizeof(usb_endpoint_descriptor_t));
}

static unsigned _fill_if_desc(usb_device_interface_t *iface, usb_interface_descriptor_t *if_desc, unsigned buffer_size)
{
	if (buffer_size >= sizeof(usb_interface_descriptor_t))
	{
		if_desc->NumEndpoints = 2;
		if_desc->InterfaceClass = USB_CLASS_CUSTOM;
		if_desc->InterfaceSubClass = 0xF0;	// MFi accessory
		if_desc->Protocol = 0x00;
		if_desc->InterfaceNameIndex = iface->Name.Index;
		return sizeof(usb_interface_descriptor_t);
	}
	return 0;
}

static unsigned _fill_ep_desc(usb_device_interface_t *iface, unsigned ep_index, usb_endpoint_descriptor_t *ep_desc, unsigned buffer_size)
{
	if (buffer_size < sizeof(usb_endpoint_descriptor_t))
		return 0;

	switch(ep_index)
	{
		case 0:	// tx (in) ep
			ep_desc->Address = 0x80 | IAP2_BULK_EP; 
			ep_desc->Attributes = USB_TT_BULK;
			ep_desc->MaxPacketSize = HTOUSB16(IAP2_MAX_PACKET_LENGTH);
			ep_desc->Interval = 0;
			return sizeof(usb_endpoint_descriptor_t);
		case 1:	// rx (out) ep
			ep_desc->Address = IAP2_BULK_EP;
			ep_desc->Attributes = USB_TT_BULK;
			ep_desc->MaxPacketSize = HTOUSB16(IAP2_MAX_PACKET_LENGTH);
			ep_desc->Interval = 0;
			return sizeof(usb_endpoint_descriptor_t);
	}
	return 0;
}

static iap2_device_context_t *_get_context(unsigned if_num)
{
	ASSERT(_initialized, KERNEL_ERROR_KERNEL_PANIC);

	iap2_device_context_t *found = NULL;

	FOREACH(n, &_instance_list)
	{
		iap2_device_context_t *iap2dev = (iap2_device_context_t *)n;
		usb_device_interface_t *iface = iap2dev->Interface;
		ASSERT(iface != NULL, KERNEL_ERROR_NULL_POINTER);
		ASSERT(iface->Status == USB_IFSTA_STARTED, KERNEL_ERROR_KERNEL_PANIC);
		if (iface->Index == if_num)
		{
			found = iap2dev;
			break;
		}
	}

	return found;
}


static void _tx_dispatch(dispatcher_context_t *context, dispatcher_t *dispatcher)
{
	iap2_device_context_t *iap2dev = (iap2_device_context_t *)dispatcher->CallbackState;

	usb_io_status_t sta = iap2dev->TxIo.Status;
	if (sta == USB_IOSTA_IN_WAIT || sta == USB_IOSTA_IN_COMPLETE)
	{
		if (iap2dev->Ready)
		{
#ifdef IAP2_DEBUG
//			ASSERT(ftdi->Interface != nullptr, KERNEL_ERROR_NULL_POINTER);
//			VERBOSE("[%d] busy", ftdi->Interface->Index);
#endif
			// continue waiting
			iap2dev->Idle = false;
			exos_dispatcher_add(context, &iap2dev->TxDispatcher, 1000);
		}
		else
		{
			ASSERT(iap2dev->Interface != NULL, KERNEL_ERROR_NULL_POINTER);
			_verbose(VERBOSE_ERROR, "[%d] timeout error! (stop)", iap2dev->Interface->Index);
			iap2dev->TxIo.Status = USB_IOSTA_UNKNOWN;
			//iap2dev->TxSize = 0;
		}
	}
	else if (sta == USB_IOSTA_ERROR)
	{
		ASSERT(iap2dev->Interface != NULL, KERNEL_ERROR_NULL_POINTER);
		_verbose(VERBOSE_ERROR, "[%d] tx error! (stop)", iap2dev->Interface->Index);
		iap2dev->TxIo.Status = USB_IOSTA_UNKNOWN;
		//iap2dev->TxSize = 0;
	}
	else
	{
		unsigned done = exos_io_buffer_read(&iap2dev->Output, iap2dev->TxData, sizeof(iap2dev->TxData));
		if (done != 0)
		{
#ifdef IAP2_DEBUG
			//static unsigned char vbuf[IAP2_MAX_PACKET_LENGTH + 1];
			//for(unsigned i = 0; i < done; i++)
			//{
			//	unsigned char c = iap2dev->TxData[2 + i];
			//	vbuf[i] = (c >= ' ' && c <= 'z') ? c : '.';
			//}
			//vbuf[done] = '\0';	
			//_verbose(VERBOSE_DEBUG,"[%d] tx '%s'", iap2dev->Interface->Index, vbuf);
#endif
			iap2dev->Idle = false;

			iap2dev->TxIo.Data = iap2dev->TxData;
			iap2dev->TxIo.Length = done;
			usb_set_tx_buffer(IAP2_BULK_EP, &iap2dev->TxIo);
		}
		else
		{
#ifdef IAP2_DEBUG
			//if (!iap2dev->Idle) _verbose(VERBOSE_DEBUG, "[%d] idle", iap2dev->Interface->Index);
#endif
			iap2dev->Idle = true;
		}

		exos_dispatcher_add(context, &iap2dev->TxDispatcher, iap2dev->Idle ? EXOS_TIMEOUT_NEVER : 100);
	}
}

static void _rx_dispatch(dispatcher_context_t *context, dispatcher_t *dispatcher)
{
	iap2_device_context_t *iap2dev = (iap2_device_context_t *)dispatcher->CallbackState;
	if (iap2dev->RxIo.Status == USB_IOSTA_DONE)
	{
#ifdef USB_DEVICE_DEBUG
		_verbose(VERBOSE_DEBUG, "rx (%d bytes)", iap2dev->RxIo.Done);
#endif
		iap2_input(&iap2dev->Transport, iap2dev->RxData, iap2dev->RxIo.Done);

		iap2dev->RxIo.Data = iap2dev->RxData;
		iap2dev->RxIo.Length = sizeof(iap2dev->RxData);
		usb_set_rx_buffer(IAP2_BULK_EP, &iap2dev->RxIo);

		exos_dispatcher_add(context, &iap2dev->RxDispatcher, EXOS_TIMEOUT_NEVER);
	}
	else
	{
		ASSERT(iap2dev->Interface != NULL, KERNEL_ERROR_NULL_POINTER);
		_verbose(VERBOSE_COMMENT, "[%d] rx stopped! -----", iap2dev->Interface->Index);
	}
}

static bool _start(usb_device_interface_t *iface, unsigned char alternate_setting, dispatcher_context_t *context)
{
	// NOTE: alternate setting is ignored

	iap2_device_context_t *iap2dev = (iap2_device_context_t *)iface->DriverContext;
	ASSERT(!list_find_node(&_instance_list, &iap2dev->Node), KERNEL_ERROR_KERNEL_PANIC);
	list_add_tail(&_instance_list, &iap2dev->Node);
	iap2dev->DispatcherContext = context;
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);

	exos_dispatcher_create(&iap2dev->RxDispatcher, &iap2dev->RxEvent, _rx_dispatch, iap2dev);
	exos_dispatcher_add(iap2dev->DispatcherContext, &iap2dev->RxDispatcher, EXOS_TIMEOUT_NEVER);
	iap2dev->RxIo = (usb_io_buffer_t) { .Event = &iap2dev->RxEvent,
		.Data = iap2dev->RxData, .Length = sizeof(iap2dev->RxData) };	// FIXME: buffer size can span multiple packets
	usb_set_rx_buffer(IAP2_BULK_EP, &iap2dev->RxIo);

	iap2dev->TxIo = (usb_io_buffer_t) { .Event = &iap2dev->TxEvent };
	exos_dispatcher_create(&iap2dev->TxDispatcher, &iap2dev->TxEvent, _tx_dispatch, iap2dev);
	exos_dispatcher_add(iap2dev->DispatcherContext, &iap2dev->TxDispatcher, EXOS_TIMEOUT_NEVER);
	exos_io_buffer_create(&iap2dev->Output, iap2dev->OutputBuffer, sizeof(iap2dev->OutputBuffer), NULL, NULL);

	_verbose(VERBOSE_COMMENT, "[%d] USB start -------", iface->Index);
	
	iap2_transport_t *t = &iap2dev->Transport;
	iap2_transport_create(t, "Host Mode", iface->Index, &_iap2_driver);

	// transport link params
	t->LinkParams.RetransmitTimeout = 2000;
	t->LinkParams.CumulativeAckTimeout = 22;
	t->LinkParams.MaxRetransmits = 30;
	t->LinkParams.MaxCumulativeAcks = 3;

	iap2_start(t);

	return true;
}

static void _stop(usb_device_interface_t *iface)
{
	iap2_device_context_t *iap2dev = (iap2_device_context_t *)iface->DriverContext;
	ASSERT(iap2dev != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(iap2dev->Interface == iface, KERNEL_ERROR_NULL_POINTER);

	ASSERT(list_find_node(&_instance_list, &iap2dev->Node), KERNEL_ERROR_KERNEL_PANIC);

	exos_mutex_lock(&iap2dev->Lock);
	//if (iap2dev->Entry != NULL)
	//{
	//	_verbose(VERBOSE_DEBUG, "[%d] closing io due to extraction", iap2dev->Interface->Index);
	//	_close(iap2dev->Entry);
	//}
	iap2dev->Ready = false;
	exos_mutex_unlock(&iap2dev->Lock);

	// TODO: remove dispatchers
	exos_dispatcher_remove(iap2dev->DispatcherContext, &iap2dev->RxDispatcher);
	exos_dispatcher_remove(iap2dev->DispatcherContext, &iap2dev->TxDispatcher);

	list_remove(&iap2dev->Node);

	_verbose(VERBOSE_COMMENT, "[%d] USB stop -------", iface->Index);
}

static bool _iap2_send(iap2_transport_t *t, const unsigned char *data, unsigned length)
{
	iap2_device_context_t *iap2dev = _get_context(t->Unit);
	ASSERT(iap2dev != NULL, KERNEL_ERROR_NULL_POINTER);

	unsigned done = exos_io_buffer_write(&iap2dev->Output, data, length);
	exos_event_set(&iap2dev->TxEvent);

	return (done == length);
}

static unsigned short _iap2_identify(iap2_transport_t *t, iap2_control_parameters_t *params)
{
	// add transport component parameters
	iap2_short_t *cid = iap2_helper_add_parameter(params, IAP2_TCID_ComponentIdentifier, sizeof(unsigned short));
	*cid = HTOIAP2S(t->ComponentId);
	iap2_helper_add_param_string(params, IAP2_TCID_ComponentName, "iAP2 USB-Host");
	iap2_helper_add_parameter(params, IAP2_TCID_SupportsiAP2Connection, 0);		 
	return IAP2_IIID_USBHostTransportComponent;
}

//static void _set_latency(ftdi_context_t *ftdi, unsigned latency)
//{
//	ASSERT(ftdi != NULL, KERNEL_ERROR_NULL_POINTER);
//	ASSERT(ftdi->Interface != NULL, KERNEL_ERROR_NULL_POINTER);

//	if (latency != 0 && latency <= 31)
//	{
//		ftdi->Latency = latency;
//	}
//	else
//	{
//		VERBOSE("set latency ignored (illegal values)");
//	}
//}

static void _reset(iap2_device_context_t *iap2dev)
{
//	iap2dev->Idle = false;
//	exos_dispatcher_add(iap2dev->DispatcherContext, &iap2dev->TxDispatcher, iap2dev->Latency);
}

bool iap2_vendor_request(usb_request_t *req, void **pdata, int *plength)
{
	static unsigned _req_num = 0;
//	VERBOSE("req #%d", _req_num++);

	iap2_device_context_t *iap2dev = _get_context(req->Index & 0xFF);
    if (iap2dev == NULL)
	{
		_verbose(VERBOSE_ERROR, "[%d] FAILED (interface not present)", req->Index & 0xFF);
		return false;
	}

	unsigned char *data = (unsigned char *)*pdata;
	unsigned short modem;

	switch(req->RequestCode)
	{
		case IAP2_VENDOR_RESET:
			_verbose(VERBOSE_DEBUG, "[%d] reset, %d", req->Index, req->Value);
			_reset(iap2dev);
			return true;
		//case FTDI_VENDOR_SET_LATENCY:
		//	VERBOSE("[%d] set latency, %d", req->Index, req->Value);
		//	_set_latency(ftdi, req->Value);
		//	return true;
	}
	_verbose(VERBOSE_DEBUG, "[%d] unk req code $%02x", req->RequestCode);
	return false;
}

#if 0
static io_error_t _open(io_entry_t *io, const char *path, io_flags_t flags)
{
	iap2_device_context_t *iap2dev = (iap2_device_context_t *)io->DriverContext;
	io_error_t res;

	exos_mutex_lock(&iap2dev->Lock);
	if (iap2dev->Entry == NULL)
	{
		ASSERT(iap2dev->Interface != NULL, KERNEL_ERROR_NULL_POINTER);
		if (iap2dev->Interface->Status == USB_IFSTA_STARTED)
		{
			if (iap2dev->Ready)
			{
				exos_io_buffer_create(&iap2dev->Output, iap2dev->OutputBuffer, IAP2_OUTPUT_BUFFER, NULL, &io->OutputEvent);
				exos_io_buffer_create(&iap2dev->Input, iap2dev->InputBuffer, IAP2_INPUT_BUFFER, &io->InputEvent, NULL);

				iap2dev->Entry = io;
				io->DriverContext = iap2dev;

				exos_event_set(&io->OutputEvent);	// FIXME
				res = IO_OK;
			}
			else res = IO_ERROR_IO_ERROR;
		}
		else res = IO_ERROR_DEVICE_NOT_MOUNTED;
	}
	else res = IO_ERROR_ALREADY_LOCKED;
	
	exos_mutex_unlock(&iap2dev->Lock);
	return res;
}

static void _close(io_entry_t *io)
{
	iap2_device_context_t *iap2dev = (iap2_device_context_t *)io->DriverContext;
	ASSERT(iap2dev != NULL, KERNEL_ERROR_NULL_POINTER);

	exos_mutex_lock(&iap2dev->Lock);
	ASSERT(iap2dev->Entry == io || iap2dev->Entry == NULL, KERNEL_ERROR_KERNEL_PANIC);
	iap2dev->Entry = NULL;
	exos_event_reset(&io->InputEvent);
	exos_event_reset(&io->OutputEvent);
   	exos_mutex_unlock(&iap2dev->Lock);
}

static int _read(io_entry_t *io, unsigned char *buffer, unsigned int length)
{
	iap2_device_context_t *iap2dev = (iap2_device_context_t *)io->DriverContext;
	ASSERT(iap2dev != nullptr, KERNEL_ERROR_NULL_POINTER);
	if (iap2dev->Entry != io)
		return -1;

	return exos_io_buffer_read(&iap2dev->Input, buffer, length);
}

static int _write(io_entry_t *io, const unsigned char *buffer, unsigned int length)
{
	iap2_device_context_t *iap2dev = (iap2_device_context_t *)io->DriverContext;
	ASSERT(iap2dev != nullptr, KERNEL_ERROR_NULL_POINTER);
	if (iap2dev->Entry != io || !iap2dev->Ready)
		return -1;

	return exos_io_buffer_write(&iap2dev->Output, buffer, length);
}
#endif




