#include "device.h"
#include "configuration.h"
#include <support/usb/device_hal.h>
#include <kernel/dispatch.h>
#include <kernel/mutex.h>
#include <kernel/panic.h>
#include <kernel/verbose.h>
#include <string.h>
#include <stdio.h>

#ifndef USB_THREAD_STACK
#define USB_THREAD_STACK 1280
#endif

static exos_thread_t _thread;
#ifdef USB_THREAD_STACK_ATTRIBUTES
static unsigned char _stack[USB_THREAD_STACK] USB_THREAD_STACK_ATTRIBUTES;
#else
static unsigned char _stack[USB_THREAD_STACK];
#endif

static void *_service(void *arg);
static event_t _control_event;
static mutex_t _device_lock;

static void _setup_handler(dispatcher_context_t *context, dispatcher_t *dispatcher);
static bool _setup_req(usb_request_t *req, void **pdata, unsigned *plength);
static bool _setup_std_req(usb_request_t *req, void **pdata, unsigned *plength);
static bool _setup_class_req(usb_request_t *req, void **pdata, unsigned *plength);
static bool _setup_vendor_req(usb_request_t *req, void **pdata, unsigned *plength);
static void _setup_error(KERNEL_ERROR error);
static void _stop_configuration(usb_device_configuration_t *conf);

static usb_request_t _setup;
static usb_io_buffer_t _setup_io;
static usb_io_buffer_t _control_in;
static usb_io_buffer_t _control_out;
static unsigned char _setup_data[8];
static unsigned char _control_data[USB_CONTROL_MAX_DATA];
static unsigned short _rx_max_packet[USB_DEVICE_ENDPOINTS];
static unsigned short _tx_max_packet[USB_DEVICE_ENDPOINTS];

static usb_device_configuration_t *_configuration = nullptr;
static unsigned char _address = 0, _hal_addr = 0; 
static unsigned char _configuration_value = 0;
static unsigned char _alternate_setting = 0;
static usb_status_t _device_status = USB_STATUS_SELF_POWERED;
static dispatcher_context_t _context;
static bool _error_state;

bool usb_device_initialize()
{
	usb_device_config_initialize();

	event_t start_event;
	exos_event_create(&start_event, EXOS_EVENTF_NONE);
	exos_thread_create(&_thread, 1, _stack, USB_THREAD_STACK, _service, &start_event);
	exos_event_wait(&start_event, EXOS_TIMEOUT_NEVER);

	return true;
}

#ifdef USB_DEVICE_DEBUG
#define _verbose(prefix, data, length) _debug(prefix, data, length)

static void _debug(const char *prefix, const void *data, unsigned length)
{
	static char buf[256];

	if (data != nullptr)
	{
		unsigned size = 0;
		for (unsigned off = 0; off < length; off++)
			size += sprintf(buf + size, off == 0 ? "%02x" : "-%02x", ((unsigned char *)data)[off]);
		buf[size] = '\0';
		verbose(VERBOSE_DEBUG, "usb_device", length != 0 ? "%s (%d b): %s" : "%s (%d b)", prefix, length, buf); 
	}
	else verbose(VERBOSE_DEBUG, "usb_device", length != 0 ? "%s (%d)" : "%s", prefix, length);
}

#else
#define _verbose(prefix, data, length)  { /* nothing */ }
#endif

static void *_service(void *arg)
{
	event_t *start_event = (event_t *)arg;

	exos_mutex_create(&_device_lock);
	exos_event_create(&_control_event, EXOS_EVENTF_AUTORESET);

	exos_dispatcher_context_create(&_context);
	dispatcher_t setup_dispatcher;
	exos_dispatcher_create(&setup_dispatcher, &_control_event, _setup_handler, nullptr);

	hal_usbd_initialize();
	exos_thread_sleep(10);

	_control_out = (usb_io_buffer_t) { .Event = &_control_event, .Flags = USB_IOF_SHORT_PACKET_END };
	_control_in = (usb_io_buffer_t) { .Event = &_control_event, .Flags = USB_IOF_SHORT_PACKET_END };
	_setup_io = (usb_io_buffer_t) { .Event = &_control_event, .Data = _setup_data, .Length = 8 };

	exos_event_set(start_event);

	while(1)
	{
		_address = _hal_addr = 0;
		_configuration = nullptr;
		_configuration_value = 0;
		_alternate_setting = 0;
		_error_state = false;

		exos_dispatcher_add(&_context, &setup_dispatcher, EXOS_TIMEOUT_NEVER);

		hal_usbd_prepare_setup_ep(&_setup_io);
		hal_usbd_connect(true);
		_verbose("connect! -----", nullptr, 0);

		while(1)
		{
			exos_dispatch(&_context, EXOS_TIMEOUT_NEVER);
		
			if (_error_state)
				break;
		}

    	if (_configuration != nullptr)
		{
			_verbose("disconnect!", nullptr, 0);
			_stop_configuration(_configuration);
		}
		else
		{
			_verbose("reset!", nullptr, 0);
		}
	}
}

static void _setup_handler(dispatcher_context_t *context, dispatcher_t *dispatcher)
{
	if (_setup_io.Status == USB_IOSTA_DONE)
	{
		exos_mutex_lock(&_device_lock);
		memcpy(&_setup, _setup_data, 8); 
		hal_usbd_prepare_setup_ep(&_setup_io);
		//_verbose("setup", &_setup, 8);

		if ((_setup.RequestType & USB_REQTYPE_DIRECTION_MASK) == USB_REQTYPE_HOST_TO_DEVICE) 
		{
			if (_setup.Length != 0)
			{
				_control_out.Data = _control_data;
				_control_out.Length = _setup.Length;
				hal_usbd_prepare_out_ep(0, &_control_out);
				_verbose("-> data_out", nullptr, 0);

				exos_event_wait(_control_out.Event, EXOS_TIMEOUT_NEVER);
				if (_control_out.Status != USB_IOSTA_DONE)
				{
					_verbose("data out failed", nullptr, 0);
					_error_state = true;
				}
			}
		}

		if (!_error_state)
		{
			//_verbose("-> process request", nullptr, 0);
			unsigned length = _setup.Length;
			void *data = _control_data;
			bool done = _setup_req(&_setup, &data, &length);

			if ((_setup.RequestType & USB_REQTYPE_DIRECTION_MASK) == USB_REQTYPE_DEVICE_TO_HOST)
			{
				if (done)
				{
					if (length > _setup.Length) length = _setup.Length;
					_control_in.Data = data;
					_control_in.Length = length;
					hal_usbd_prepare_in_ep(0, &_control_in);
					//_verbose("-> data_in", data, length);

					exos_event_wait(_control_in.Event, EXOS_TIMEOUT_NEVER);
					if (_control_in.Status != USB_IOSTA_DONE)
					{
						_verbose("data in failed", nullptr, 0);
                        _error_state = true;
					}
				}
				else
				{
					_verbose("-> stall in (FAIL)", nullptr, 0);
					hal_usbd_stall_in_ep(0, true);
				}
			}
			else
			{
				if (done)
				{
					_control_in.Data = _control_data;
					_control_in.Length = 0;
					hal_usbd_prepare_in_ep(0, &_control_in);
					//_verbose("-> status_in", nullptr, 0);

					exos_event_wait(_control_in.Event, EXOS_TIMEOUT_NEVER);
					if (_control_in.Status != USB_IOSTA_DONE)
					{
						_verbose("status in failed", nullptr, 0);
                        _error_state = true;
					}
				}
				else
				{
					_verbose("-> stall out (FAIL)", nullptr, 0);
					hal_usbd_stall_out_ep(0, true);
				}
			}

			if (_address != _hal_addr)
			{
				// calls hal layer if setup has set a new address
				hal_usbd_set_address(_address);
				_hal_addr = _address;
			}
		}

		exos_mutex_unlock(&_device_lock);
	}
	else
	{
		if (_setup_io.Status == USB_IOSTA_ERROR)
		{
			_error_state = true;
		}
	}

	exos_dispatcher_add(context, dispatcher, EXOS_TIMEOUT_NEVER);
}


static void _stop_configuration(usb_device_configuration_t *conf)
{
	ASSERT(conf != nullptr, KERNEL_ERROR_NULL_POINTER);

	const usb_device_configuration_driver_t *driver = conf->Driver;
	if (driver == nullptr)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);

	FOREACH(node, &conf->Interfaces)
	{
		usb_device_interface_t *if_node = (usb_device_interface_t *)node;
		const usb_device_interface_driver_t *if_driver = if_node->Driver;
		if (if_driver == nullptr)
			kernel_panic(KERNEL_ERROR_NULL_POINTER);
		
		if (if_node->Status == USB_IFSTA_STARTED)
		{
			if (if_driver->Stop != nullptr)
			{
				if_node->Status = USB_IFSTA_STOPPING;
				if_driver->Stop(if_node);
			}
            if_node->Status = USB_IFSTA_STOPPED;
		}
	}

	if (driver->Configured != nullptr)
		driver->Configured(conf->Index, false);

	_configuration_value = 0;
	_configuration = nullptr;
	hal_usbd_configure(false);
}

static void _set_configuration(unsigned char conf_value)
{
    usb_device_configuration_t *conf;
	unsigned conf_index = 0;
	while (conf = usb_device_config_get(conf_index))
	{
		if (conf->Value == conf_value)
		{
			const usb_device_configuration_driver_t *driver = conf->Driver;
			if (driver == nullptr)
				kernel_panic(KERNEL_ERROR_NULL_POINTER);

			int if_configured = 0;
			FOREACH(node, &conf->Interfaces)
			{
				usb_device_interface_t *if_node = (usb_device_interface_t *)node;
				const usb_device_interface_driver_t *if_driver = if_node->Driver;
				if (if_driver == nullptr)
					kernel_panic(KERNEL_ERROR_NULL_POINTER);

				usb_endpoint_descriptor_t ep_desc;
				for (int ep_index = 0; ; ep_index++)
				{
					int size = if_driver->FillEndpointDescriptor(if_node, ep_index, &ep_desc, sizeof(usb_endpoint_descriptor_t));
					if (size == 0) break;

					// save max packet length
					int ep_num = ep_desc.AddressBits.EndpointNumber;
					if (ep_desc.AddressBits.Input)
						_tx_max_packet[ep_num] = USB16TOH(ep_desc.MaxPacketSize);
					else
						_rx_max_packet[ep_num] = USB16TOH(ep_desc.MaxPacketSize);

					if (ep_desc.AddressBits.Input)
					{
						hal_usbd_enable_in_ep(ep_desc.AddressBits.EndpointNumber, 
							ep_desc.AttributesBits.TransferType, 
							USB16TOH(ep_desc.MaxPacketSize));
					}
					else
					{
						hal_usbd_enable_out_ep(ep_desc.AddressBits.EndpointNumber, 
							ep_desc.AttributesBits.TransferType, 
							USB16TOH(ep_desc.MaxPacketSize));
					}
				}

				if_node->Status = USB_IFSTA_STARTING;
				if (if_driver->Start(if_node, _alternate_setting, &_context))
				{
					if_configured++;
					if_node->Status = USB_IFSTA_STARTED;
				}
			}

			if (if_configured != 0) 
			{
				_configuration_value = conf_value;
                _configuration = conf;
				hal_usbd_configure(true);

				if (driver->Configured != nullptr)
					driver->Configured(conf_index, true);
				break;
			}

		}
		conf_index++;
	}
}

static usb_device_interface_t *_get_conf_interface(usb_device_configuration_t *conf, unsigned index)
{
	usb_device_interface_t *found = nullptr;
	FOREACH(node, &conf->Interfaces)
	{
		usb_device_interface_t *iface = (usb_device_interface_t *)node;
		if (iface->Index == index)
		{
			found = iface;
			break;
		}
	}
	return found;
}

static usb_device_interface_t *_get_interface(unsigned index)
{
	ASSERT(_configuration != nullptr, KERNEL_ERROR_KERNEL_PANIC);
	return _get_conf_interface(_configuration, index);
}

static bool _setup_req(usb_request_t *req, void **pdata, unsigned *plength)
{
	unsigned length = *plength;
	bool done = false;

	switch(_setup.RequestType & USB_REQTYPE_MASK)
	{
		case USB_REQTYPE_STANDARD:
			done = _setup_std_req(&_setup, pdata, &length);
			break;
		case USB_REQTYPE_CLASS:
			done = _setup_class_req(&_setup, pdata, &length);
			break;
		case USB_REQTYPE_VENDOR:
			done = _setup_vendor_req(&_setup, pdata, &length);
			break;
	}
	if (done) *plength = length;
	return done;
}

static bool _setup_std_req(usb_request_t *req, void **pdata, unsigned *plength)
{
	usb_request_code_t code = req->RequestCode;
	usb_recipient_t recipient = req->RequestType & USB_REQTYPE_RECIPIENT_MASK;
	if (recipient == USB_REQTYPE_RECIPIENT_DEVICE)
	{
		switch(code)
		{
			case USB_REQUEST_GET_DESCRIPTOR:
//				_verbose("get_descriptor", req, 8);
				*pdata = usb_device_config_get_descriptor(req->Value, req->Index, plength); 
				if (*pdata == nullptr)
					_verbose("get_descriptor returned NULL", req, 8);
				return (*pdata != nullptr);
			case USB_REQUEST_SET_ADDRESS:
				_verbose("set_address", &req->Value, 2);
				_address = req->Value & 0x7f;			// NOTE: will call hal layer after request handshake
				hal_usbd_set_address_early(_address);	// NOTE: some cores (lpc17/stm32_fs) use this call
				return true;
			case USB_REQUEST_SET_CONFIGURATION:
				_verbose("set_configuration", &req->Value, 2);
				_set_configuration(req->Value & 0xff);
				return true;
			case USB_REQUEST_GET_STATUS:
				*pdata = &_device_status;
				*plength = 2;
				return true;
			default:
				_setup_error(KERNEL_ERROR_NOT_IMPLEMENTED);
				break;
		}
	}
	else if (recipient == USB_REQTYPE_RECIPIENT_INTERFACE)
	{
		_verbose("interface request", req, 8);
		usb_device_interface_t *iface = _get_interface(req->Index);
		ASSERT(iface != nullptr, KERNEL_ERROR_KERNEL_PANIC);
		const usb_device_interface_driver_t *driver = iface->Driver;
		ASSERT(driver != nullptr, KERNEL_ERROR_NULL_POINTER);
		if (driver->InterfaceRequest != nullptr)
			return driver->InterfaceRequest(iface, req, pdata, plength);
	}
	else if (recipient == USB_REQTYPE_RECIPIENT_ENDPOINT)
	{
		int ep_num = req->Index & 0x3f;
		switch(code)
		{
			case USB_REQUEST_CLEAR_FEATURE:
				if (req->Value == 0)
				{
					if (req->Index & 0x80)
					{
						_verbose("clear_halt in ep", nullptr, ep_num);
						hal_usbd_stall_in_ep(ep_num, false);
					}
					else
					{
						_verbose("clear_halt out ep", nullptr, ep_num);
						hal_usbd_stall_out_ep(ep_num, false);
					}
				}
				return true;
			default:
				_setup_error(KERNEL_ERROR_NOT_IMPLEMENTED);
				break;
		}
	}
	return false;
}

static bool _setup_class_req(usb_request_t *req, void **pdata, unsigned *plength)
{
	usb_recipient_t recipient = req->RequestType & USB_REQTYPE_RECIPIENT_MASK;
	if (recipient == USB_REQTYPE_RECIPIENT_DEVICE)
	{
		if (_configuration != NULL)
		{
			const usb_device_configuration_driver_t *driver = _configuration->Driver;
			ASSERT(driver != nullptr, KERNEL_ERROR_NULL_POINTER);
			if (driver->DeviceClassRequest != nullptr)
			{
				_verbose("class device request", req, 8);
				return driver->DeviceClassRequest(req, pdata, plength);
			}
			_verbose("class device request unhandled (FAIL)", req, 8);
			return false;
		}
		else 
		{
			_verbose("class device request called before SetConfiguration() (FAIL)", req, 8);
			return false;
		}
	}
	else if (recipient == USB_REQTYPE_RECIPIENT_INTERFACE)
	{
		_verbose("class interface request", req, 8);
		usb_device_interface_t *iface = _get_interface(req->Index);
		ASSERT(iface != nullptr, KERNEL_ERROR_KERNEL_PANIC);
		const usb_device_interface_driver_t *driver = iface->Driver;
		ASSERT(driver != nullptr, KERNEL_ERROR_NULL_POINTER);
		if (driver->ClassRequest != nullptr)
			return driver->ClassRequest(iface, req, pdata, plength);
	}
	return false;
}

static bool _setup_vendor_req(usb_request_t *req, void **pdata, unsigned *plength)
{
	usb_recipient_t recipient = req->RequestType & USB_REQTYPE_RECIPIENT_MASK;
	if (recipient == USB_REQTYPE_RECIPIENT_DEVICE)
	{
		if (_configuration != NULL)
		{
			const usb_device_configuration_driver_t *driver = _configuration->Driver;
			ASSERT(driver != nullptr, KERNEL_ERROR_NULL_POINTER);
			if (driver->DeviceVendorRequest != nullptr)
			{
				_verbose("vendor device request", req, 8);
				return driver->DeviceVendorRequest(req, pdata, plength);
			}
			_verbose("vendor device request unhandled (FAIL)", req, 8);
			return false;
		}
		else 
		{
			_verbose("vendor device request called before SetConfiguration() (FAIL)", req, 8);
			return false;
		}
	}
	else if (recipient == USB_REQTYPE_RECIPIENT_INTERFACE)
	{
		_verbose("vendor interface request", req, 8);
		usb_device_interface_t *iface = _get_interface(req->Index);
		ASSERT(iface != nullptr, KERNEL_ERROR_KERNEL_PANIC);
		const usb_device_interface_driver_t *driver = iface->Driver;
		ASSERT(driver != nullptr, KERNEL_ERROR_NULL_POINTER);
		if (driver->VendorRequest != nullptr)
			return driver->VendorRequest(iface, req, pdata, plength);
	}
	return false;
}

void usb_set_rx_buffer(unsigned ep_num, usb_io_buffer_t *buffer)
{
	ASSERT(buffer != nullptr && buffer->Event != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(buffer->Event->Flags & EXOS_EVENTF_AUTORESET, KERNEL_ERROR_KERNEL_PANIC);
	ASSERT(ep_num < USB_DEVICE_ENDPOINTS, KERNEL_ERROR_KERNEL_PANIC);
	if (ep_num != 0)
	{
		exos_mutex_lock(&_device_lock);
		hal_usbd_prepare_out_ep(ep_num, buffer);
		exos_mutex_unlock(&_device_lock);
	}
}

void usb_set_tx_buffer(unsigned ep_num, usb_io_buffer_t *buffer)
{
	ASSERT(buffer != nullptr && buffer->Event != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(buffer->Event->Flags & EXOS_EVENTF_AUTORESET, KERNEL_ERROR_KERNEL_PANIC);
	ASSERT(ep_num < USB_DEVICE_ENDPOINTS, KERNEL_ERROR_KERNEL_PANIC);
	if (ep_num != 0)
	{
		exos_mutex_lock(&_device_lock);
		hal_usbd_prepare_in_ep(ep_num, buffer);
		exos_mutex_unlock(&_device_lock);
	}
}

static void _setup_error(KERNEL_ERROR error)
{
	// NOTE: we will NOT panic in release mode
#ifdef DEBUG
	kernel_panic(error);
#endif
}
