#include "ftdi.h"
#include <support/usb/device_hal.h>
#include <support/uart_hal.h>
#include <kernel/memory.h>
#include <kernel/dispatch.h>
#include <kernel/panic.h>
#include <stdio.h>
#include <kernel/verbose.h>

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
const usb_device_interface_driver_t *__usb_ftdi_driver = &_driver;


static io_error_t _open(io_entry_t *io, const char *path, io_flags_t flags);
static void _close(io_entry_t *io);
static int _read(io_entry_t *io, unsigned char *buffer, unsigned int length);
static int _write(io_entry_t *io, const unsigned char *buffer, unsigned int length);
static const io_driver_t _io_driver = {
	.Open = _open, .Close = _close,
	.Read = _read, .Write = _write };

#define FTDI_EEPROM_SIZE 128
static unsigned char _eeprom[FTDI_EEPROM_SIZE];

static bool _initialized = false;
static list_t _instance_list;
static unsigned _device_count = 0;
static unsigned _fill_string(int *poffset, const char *s);

#ifndef FTDI_BULK_EP
#define FTDI_BULK_EP 2	// FIXME: allocate
#endif

#ifdef FTDI_ENABLE_DEBUG
#define VERBOSE(...) verbose(VERBOSE_DEBUG, "ftdi", __VA_ARGS__)
#else 
#define VERBOSE(...) { /* nothing */ }
#endif

static bool _initialize(usb_device_interface_t *iface, const void *instance_data)
{
	usb_device_config_add_string(&iface->Name, "FTDI Interface");

	if (!_initialized)
	{
		list_initialize(&_instance_list);
		_initialized = true;

		ftdi_eeprom_t *eeprom = (ftdi_eeprom_t *)_eeprom;
		eeprom->HighCurrent = 0; 
		eeprom->EndpointSize = FTDI_MAX_PACKET_LENGTH;
		eeprom->VendorID = HTOUSB16(USB_USER_DEVICE_VENDORID);
		eeprom->ProductID = HTOUSB16(USB_USER_DEVICE_PRODUCTID);
		eeprom->ProductVersion = HTOUSB16(USB_USER_DEVICE_VERSION);
		eeprom->UsbFeatures = 0xA0;	// FIXME
		eeprom->UsbMaxPower = 0x2D;	// FIXME
		eeprom->SuspendPulldown = 0x08; // FIXME
		eeprom->Invert = 0x00;	// FIXME
		eeprom->UsbVersion = HTOUSB16(0);

		int offset = sizeof(ftdi_eeprom_t);
		eeprom->ManufacturerOffset = offset | 0x80;
		eeprom->ManufacturerLength = _fill_string(&offset, USB_USER_DEVICE_MANUFACTURER);
		eeprom->ProductOffset = offset | 0x80 ;
		eeprom->ProductLength = _fill_string(&offset, USB_USER_DEVICE_PRODUCT);
		eeprom->SerialOffset = offset | 0x80;
		eeprom->SerialLength = _fill_string(&offset, USB_USER_DEVICE_SERIALNUMBER);

		eeprom->CBusFunc[0] = 0x23;	// FIXME
		eeprom->CBusFunc[1] = 0x10;	// FIXME
		eeprom->CBusFunc[2] = 0x05;	// FIXME
		eeprom->CBusFunc[3] = 0x00;	// FIXME

		// TODO: calculate checksum
	}

	ftdi_context_t *ftdi = (ftdi_context_t *)exos_mem_alloc(sizeof(ftdi_context_t), EXOS_MEMF_CLEAR);
	if (ftdi != nullptr)
	{
		ftdi->Latency = 16;
		ftdi->TxSize = 0;
		exos_event_create(&ftdi->TxEvent, EXOS_EVENTF_AUTORESET);
		exos_event_create(&ftdi->RxEvent, EXOS_EVENTF_AUTORESET);

		ftdi->Interface = iface;
		iface->DriverContext = ftdi;

		exos_mutex_create(&ftdi->Lock);
		ftdi->Entry = nullptr;

		ftdi->Unit = _device_count++;
		sprintf(ftdi->DeviceName, "/dev/ftdi%d", ftdi->Unit);
		exos_io_add_device(&ftdi->DeviceNode, ftdi->DeviceName, &_io_driver, ftdi);

		return true;
	}
	else VERBOSE("not enough memory!");
	return false;
}

static unsigned _fill_string(int *poffset, const char *s)
{
	int offset = *poffset;
	for(int i = 0; (offset + i) < FTDI_EEPROM_SIZE; i++)
	{
		if (i == 0)
		{
			i++;	// skip length, will fill at the end
			_eeprom[offset + i] = 0x3;	// ?
			continue;
		}

		char c = *s++;
		if (c == '\0') 
		{
			_eeprom[offset] = i;
			*poffset = offset + i;
			return i;
		}
		_eeprom[offset + i++] = c;
		_eeprom[offset + i] = 0;
	}
	return 0;
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
		if_desc->InterfaceSubClass = 0xFF;
		if_desc->Protocol = 0xFF;
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
			ep_desc->Address = 0x80 | FTDI_BULK_EP; 
			ep_desc->Attributes = USB_TT_BULK;
			ep_desc->MaxPacketSize = HTOUSB16(FTDI_MAX_PACKET_LENGTH);
			ep_desc->Interval = 0;
			return sizeof(usb_endpoint_descriptor_t);
		case 1:	// rx (out) ep
			ep_desc->Address = FTDI_BULK_EP;
			ep_desc->Attributes = USB_TT_BULK;
			ep_desc->MaxPacketSize = HTOUSB16(FTDI_MAX_PACKET_LENGTH);
			ep_desc->Interval = 0;
			return sizeof(usb_endpoint_descriptor_t);
	}
	return 0;
}

/*
    Layout of the low byte:
    - B0..B3 - must be 0
    - B4       Clear to send (CTS)
                 0 = inactive
                 1 = active
    - B5       Data set ready (DTS)
                 0 = inactive
                 1 = active
    - B6       Ring indicator (RI)
                 0 = inactive
                 1 = active
    - B7       Receive line signal detect (RLSD)
                 0 = inactive
                 1 = active

    Layout of the high byte:
    - B0       Data ready (DR)
    - B1       Overrun error (OE)
    - B2       Parity error (PE)
    - B3       Framing error (FE)
    - B4       Break interrupt (BI)
    - B5       Transmitter holding register (THRE)
    - B6       Transmitter empty (TEMT)
    - B7       Error in RCVR FIFO
*/

static unsigned short _get_modem_status(ftdi_context_t *ftdi)
{
	//TODO
	return 0x0160;
}

static void _tx_dispatch(dispatcher_context_t *context, dispatcher_t *dispatcher)
{
	ftdi_context_t *ftdi = (ftdi_context_t *)dispatcher->CallbackState;

	usb_io_status_t sta = ftdi->TxIo.Status;
	if (sta == USB_IOSTA_IN_WAIT || sta == USB_IOSTA_IN_COMPLETE)
	{
		if (ftdi->Ready)
		{
#ifdef FTDI_ENABLE_DEBUG
			ASSERT(ftdi->Interface != nullptr, KERNEL_ERROR_NULL_POINTER);
			VERBOSE("[%d] busy", ftdi->Interface->Index);
#endif
			// continue waiting
			exos_dispatcher_add(context, &ftdi->TxDispatcher, ftdi->Latency);
		}
		else
		{
			ASSERT(ftdi->Interface != nullptr, KERNEL_ERROR_NULL_POINTER);
			VERBOSE("[%d] timeout (stop)", ftdi->Interface->Index);
			ftdi->TxIo.Status = USB_IOSTA_UNKNOWN;
			ftdi->TxSize = 0;
		}
	}
	else if (sta == USB_IOSTA_ERROR)
	{
		ASSERT(ftdi->Interface != nullptr, KERNEL_ERROR_NULL_POINTER);
		VERBOSE("[%d] tx error! (stop)", ftdi->Interface->Index);
		ftdi->TxIo.Status = USB_IOSTA_UNKNOWN;
		ftdi->TxSize = 0;
	}
	else
	{
		unsigned fit = (FTDI_MAX_PACKET_LENGTH - 2) - ftdi->TxSize;
		ASSERT(fit > 0, KERNEL_ERROR_KERNEL_PANIC);
		unsigned done = exos_io_buffer_read(&ftdi->Output, &ftdi->TxData[2 + ftdi->TxSize], fit);
		ftdi->TxSize += done;
		
		if (done == fit || dispatcher->State == DISPATCHER_TIMEOUT)
		{
			unsigned short modem = _get_modem_status(ftdi);
			ftdi->TxData[0] = modem & 0xff;
			ftdi->TxData[1] = modem >> 8;

			if (ftdi->TxSize != 0)
			{
#if defined FTDI_ENABLE_DEBUG
				static unsigned char vbuf[FTDI_MAX_PACKET_LENGTH];
				for(unsigned i = 0; i < ftdi->TxSize; i++)
				{
					unsigned char c = ftdi->TxData[2 + i];
					vbuf[i] = (c >=' ' && c <='z') ? c : '.';
				}
				vbuf[ftdi->TxSize] = '\0';	
				VERBOSE("[%d] tx %s", ftdi->Interface->Index, vbuf);
#endif
				ftdi->Idle = false;
			}

			ftdi->TxIo.Data = ftdi->TxData;
			ftdi->TxIo.Length = 2 + ftdi->TxSize;
			usb_set_tx_buffer(FTDI_BULK_EP, &ftdi->TxIo);
			
			ftdi->TxSize = 0;
		}
		else
		{
#ifdef FTDI_ENABLE_DEBUG
			if (!ftdi->Idle) VERBOSE("[%d] idle", ftdi->Interface->Index);
#endif
			ftdi->Idle = true;
		}

		exos_dispatcher_add(context, &ftdi->TxDispatcher, ftdi->Latency);
	}
}

static void _rx_dispatch(dispatcher_context_t *context, dispatcher_t *dispatcher)
{
	ftdi_context_t *ftdi = (ftdi_context_t *)dispatcher->CallbackState;
	if (ftdi->RxIo.Status == USB_IOSTA_DONE)
	{
#ifdef USB_ENABLE_DEBUG
	//	printf("r(%d) ", ftdi->RxIo.Done);
#endif
		unsigned done = exos_io_buffer_write(&ftdi->Input, ftdi->RxData, ftdi->RxIo.Done);
#ifdef USB_ENABLE_DEBUG
		ASSERT(ftdi->Interface != nullptr, KERNEL_ERROR_NULL_POINTER);
		if (done != ftdi->RxIo.Done)
			VERBOSE("[%d] rx overrun!", ftdi->Interface->Index);
#endif
		ftdi->RxIo.Data = ftdi->RxData;
		ftdi->RxIo.Length = 64;
		usb_set_rx_buffer(FTDI_BULK_EP, &ftdi->RxIo);

		exos_dispatcher_add(context, &ftdi->RxDispatcher, EXOS_TIMEOUT_NEVER);
	}
	else
	{
		ASSERT(ftdi->Interface != nullptr, KERNEL_ERROR_NULL_POINTER);
		VERBOSE("[%d] rx stopped! -----", ftdi->Interface->Index);
	}
}

static bool _start(usb_device_interface_t *iface, unsigned char alternate_setting, dispatcher_context_t *context)
{
	// NOTE: alternate setting is ignored

	ftdi_context_t *ftdi = (ftdi_context_t *)iface->DriverContext;
	ASSERT(!list_find_node(&_instance_list, &ftdi->Node), KERNEL_ERROR_KERNEL_PANIC);	// BUG: here fails when re-connected
	list_add_tail(&_instance_list, &ftdi->Node);
	ftdi->DispatcherContext = context;
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);

	exos_dispatcher_create(&ftdi->RxDispatcher, &ftdi->RxEvent, _rx_dispatch, ftdi);
	exos_dispatcher_add(ftdi->DispatcherContext, &ftdi->RxDispatcher, EXOS_TIMEOUT_NEVER);
	ftdi->RxIo = (usb_io_buffer_t) { .Event = &ftdi->RxEvent,
		.Data = ftdi->RxData, .Length = 64 };	// FIXME: ep size
	usb_set_rx_buffer(FTDI_BULK_EP, &ftdi->RxIo);

	ftdi->TxIo = (usb_io_buffer_t) { .Event = &ftdi->TxEvent };
	exos_dispatcher_create(&ftdi->TxDispatcher, &ftdi->TxEvent, _tx_dispatch, ftdi);
	// NOTE: tx dispatcher is not added until needed

	VERBOSE("[%d] USB start -------", iface->Index);
	return true;
}

static void _stop(usb_device_interface_t *iface)
{
	ftdi_context_t *ftdi = (ftdi_context_t *)iface->DriverContext;
	ASSERT(ftdi != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(ftdi->Interface == iface, KERNEL_ERROR_NULL_POINTER);

	ASSERT(list_find_node(&_instance_list, &ftdi->Node), KERNEL_ERROR_KERNEL_PANIC);

	exos_mutex_lock(&ftdi->Lock);
	if (ftdi->Entry != nullptr)
	{
		VERBOSE("[%d] closing io due to extraction", ftdi->Interface->Index);
		_close(ftdi->Entry);
	}
	ftdi->Ready = false;
	exos_mutex_unlock(&ftdi->Lock);

	// TODO: remove dispatchers
	exos_dispatcher_remove(ftdi->DispatcherContext, &ftdi->RxDispatcher);
	
	exos_dispatcher_remove(ftdi->DispatcherContext, &ftdi->TxDispatcher);

	list_remove(&ftdi->Node);

	VERBOSE("[%d] USB stop -------", iface->Index);
}

static ftdi_context_t *_get_context(unsigned if_num)
{
	ASSERT(_initialized, KERNEL_ERROR_KERNEL_PANIC);

	ftdi_context_t *found = NULL;

	FOREACH(n, &_instance_list)
	{
		ftdi_context_t *ftdi = (ftdi_context_t *)n;
		usb_device_interface_t *iface = ftdi->Interface;
		ASSERT(iface != NULL, KERNEL_ERROR_NULL_POINTER);
		ASSERT(iface->Status == USB_IFSTA_STARTED, KERNEL_ERROR_KERNEL_PANIC);
		if (iface->Index == if_num)
		{
			found = ftdi;
			break;
		}
	}

	return found;
}

static int _read_eeprom(int offset, unsigned char *data, int length)
{
	offset &= FTDI_EEPROM_SIZE - 1;
	for (int i = 0;  i < length; i++)
	{
		data[i] = _eeprom[offset++];
		offset &= FTDI_EEPROM_SIZE - 1;
	}
	return true;
}

static void _dtr(ftdi_context_t *ftdi, bool dtr)
{
	ASSERT(ftdi != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(ftdi->Interface != NULL, KERNEL_ERROR_NULL_POINTER);

	exos_mutex_lock(&ftdi->Lock);
	ftdi->Ready = dtr;
	exos_mutex_unlock(&ftdi->Lock);
}

static void _set_latency(ftdi_context_t *ftdi, unsigned latency)
{
	ASSERT(ftdi != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(ftdi->Interface != NULL, KERNEL_ERROR_NULL_POINTER);

	if (latency != 0 && latency <= 31)
	{
		ftdi->Latency = latency;
	}
	else
	{
		VERBOSE("set latency ignored (illegal values)");
	}
}

static void _reset(ftdi_context_t *ftdi)
{
	ftdi->Idle = false;
	exos_dispatcher_add(ftdi->DispatcherContext, &ftdi->TxDispatcher, ftdi->Latency);
}

bool ftdi_vendor_request(usb_request_t *req, void **pdata, int *plength)
{
#ifdef FTDI_ENABLE_DEBUG
	static char *parity[] = { "N", "O", "E", "M", "S", "(5)", "(6)", "(7)" };
	static char *stop[] = { "1", "1.5", "2", "(3)" };
#endif
	static unsigned _req_num = 0;
//	VERBOSE("req #%d", _req_num++);

	ftdi_context_t *ftdi = _get_context(req->Index & 0xFF);
    if (ftdi == nullptr)
	{
		VERBOSE("[%d] FAILED (interface not present)", req->Index & 0xFF);
		return false;
	}

	unsigned char *data = (unsigned char *)*pdata;
	unsigned short modem;

	switch(req->RequestCode)
	{
		case FTDI_VENDOR_RESET:
			VERBOSE("[%d] reset, %d", req->Index, req->Value);
			_reset(ftdi);
			return true;
		case FTDI_VENDOR_MODEM_CTRL:
#ifdef FTDI_ENABLE_DEBUG
			if ((req->Value >> 8) & FTDI_VENDOR_MODEM_CTRL_DTR)
				VERBOSE("[%d] modem control, DTR %s", req->Index, req->Value & FTDI_VENDOR_MODEM_CTRL_DTR ? "high" : "low");
			if ((req->Value >> 8) & FTDI_VENDOR_MODEM_CTRL_RTS)
				VERBOSE("[%d] modem control, RTS %s", req->Index, req->Value & FTDI_VENDOR_MODEM_CTRL_RTS ? "high" : "low");
#endif
			if ((req->Value >> 8) & FTDI_VENDOR_MODEM_CTRL_DTR)
				_dtr(ftdi, req->Value & FTDI_VENDOR_MODEM_CTRL_DTR);
			return true;
		case FTDI_VENDOR_FLOW_CTRL:
			VERBOSE("[%d] flow control, $%04x, $%02x", req->Index & 0xff, req->Value, req->Index >> 8);
			return true;
		case FTDI_VENDOR_SET_BAUDRATE:
			// TODO
			VERBOSE("[%d] set baudrate, $%04x, $%02x", req->Index & 0xff, req->Value, req->Index >> 8);
			return true;
		case FTDI_VENDOR_SET_FRAME:
			VERBOSE("[%d] set frame, %d%s%s %s", req->Index, req->Value & 0xF, 
				parity[(req->Value >> FTDI_VENDOR_SET_FRAME_PARITY_BIT) & 7],
				stop[(req->Value >> FTDI_VENDOR_SET_FRAME_STOP_BIT) & 3],
				req->Value & FTDI_VENDOR_SET_FRAME_BREAK ? "break!" : "");
			return true;
		case FTDI_VENDOR_MODEM_POLL:
			modem = _get_modem_status(ftdi);
			VERBOSE("[%d] modem poll ($%x)", req->Index, modem);
			((unsigned char *)(*pdata))[0] = modem & 0xff;
			((unsigned char *)(*pdata))[1] = modem >> 8;
			return true;
		case FTDI_VENDOR_SET_LATENCY:
			VERBOSE("[%d] set latency, %d", req->Index, req->Value);
			_set_latency(ftdi, req->Value);
			return true;
		case FTDI_VENDOR_GET_LATENCY:
			// TODO
			VERBOSE("[%d] get latency", req->Index);
			return true;
		case FTDI_VENDOR_FLUSH_DATA:
			// TODO
			VERBOSE("[%d] flush", req->Index);
			return true;
		case FTDI_VENDOR_READ_EEPROM:
			return _read_eeprom(req->Index * 2, (unsigned char *)*pdata, *plength);
		case FTDI_VENDOR_WRITE_EEPROM:
			return true;
	}
	VERBOSE("[%d] unk req code $%02x", req->RequestCode);
	return false;
}


static io_error_t _open(io_entry_t *io, const char *path, io_flags_t flags)
{
	io_error_t res;
	ftdi_context_t *ftdi = (ftdi_context_t *)io->DriverContext;

	exos_mutex_lock(&ftdi->Lock);
	if (ftdi->Entry == NULL)
	{
		ASSERT(ftdi->Interface != NULL, KERNEL_ERROR_NULL_POINTER);
		if (ftdi->Interface->Status == USB_IFSTA_STARTED)
		{
			if (ftdi->Ready)
			{
				exos_io_buffer_create(&ftdi->Output, ftdi->OutputBuffer, FTDI_OUTPUT_BUFFER, NULL, &io->OutputEvent);
				exos_io_buffer_create(&ftdi->Input, ftdi->InputBuffer, FTDI_INPUT_BUFFER, &io->InputEvent, NULL);

				ftdi->Entry = io;
				io->DriverContext = ftdi;

				exos_event_set(&io->OutputEvent);	// FIXME
				res = IO_OK;
			}
			else res = IO_ERROR_IO_ERROR;
		}
		else res = IO_ERROR_DEVICE_NOT_MOUNTED;
	}
	else res = IO_ERROR_ALREADY_LOCKED;
	
	exos_mutex_unlock(&ftdi->Lock);
	return res;
}

static void _close(io_entry_t *io)
{
	ftdi_context_t *ftdi = (ftdi_context_t *)io->DriverContext;
	ASSERT(ftdi != NULL, KERNEL_ERROR_NULL_POINTER);

	exos_mutex_lock(&ftdi->Lock);
	ASSERT(ftdi->Entry == io || ftdi->Entry == NULL, KERNEL_ERROR_KERNEL_PANIC);
	ftdi->Entry = NULL;
	exos_event_reset(&io->InputEvent);
	exos_event_reset(&io->OutputEvent);
   	exos_mutex_unlock(&ftdi->Lock);
}

static int _read(io_entry_t *io, unsigned char *buffer, unsigned int length)
{
	ftdi_context_t *ftdi = (ftdi_context_t *)io->DriverContext;
	ASSERT(ftdi != nullptr, KERNEL_ERROR_NULL_POINTER);
	if (ftdi->Entry != io)
		return -1;

	return exos_io_buffer_read(&ftdi->Input, buffer, length);
}

static int _write(io_entry_t *io, const unsigned char *buffer, unsigned int length)
{
	ftdi_context_t *ftdi = (ftdi_context_t *)io->DriverContext;
	ASSERT(ftdi != nullptr, KERNEL_ERROR_NULL_POINTER);
	if (ftdi->Entry != io || !ftdi->Ready)
		return -1;

	return exos_io_buffer_write(&ftdi->Output, buffer, length);
}




