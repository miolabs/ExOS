#include "hid.h"
#include <usb/enumerate.h>
#include <kernel/fifo.h>
#include <kernel/dispatch.h>
#include <kernel/verbose.h>
#include <kernel/panic.h>
#include <kernel/machine/hal.h>
#include <string.h>

#ifndef HID_MAX_INSTANCES
#define HID_MAX_INSTANCES 1
#endif

static hid_function_t _function[HID_MAX_INSTANCES] __usb;
static unsigned char _function_busy[HID_MAX_INSTANCES];

static usb_host_function_t *_check_interface(usb_host_device_t *device, usb_configuration_descriptor_t *conf_desc, usb_descriptor_header_t *fn_desc);
static void _start(usb_host_function_t *func, usb_configuration_descriptor_t *conf_desc, usb_descriptor_header_t *fn_desc);
static void _stop(usb_host_function_t *func);
static const usb_host_function_driver_t _driver = { 
	.CheckInterface = _check_interface, 
	.Start =_start, .Stop = _stop };
static usb_host_function_driver_node_t _driver_node;

static void _dispatch(dispatcher_context_t *context, dispatcher_t *dispatcher);

#ifndef HID_MAX_REPORT_DESCRIPTOR_SIZE
#define HID_MAX_REPORT_DESCRIPTOR_SIZE 128
#endif

static mutex_t _manager_lock;
static list_t _manager_list;
static dispatcher_context_t *_context;

void usb_hid_initialize(dispatcher_context_t *context)
{
	_context = context;
	for(unsigned i = 0; i < HID_MAX_INSTANCES; i++)
		_function[i] = (hid_function_t) { .Handler = nullptr };

	exos_mutex_create(&_manager_lock);
	list_initialize(&_manager_list);
	_driver_node = (usb_host_function_driver_node_t) { .Driver = &_driver };
	usb_host_driver_register(&_driver_node);
}

void usb_hid_add_driver(hid_driver_node_t *node)
{
	ASSERT(node != nullptr, KERNEL_ERROR_NULL_POINTER);
	exos_mutex_lock(&_manager_lock);
	list_add_tail(&_manager_list, &node->Node);
	exos_mutex_unlock(&_manager_lock);
}


static unsigned char _report_buffer[HID_MAX_REPORT_DESCRIPTOR_SIZE] __usb;

static const unsigned char _short_item_length[4] = { 0, 1, 2, 4 };

static void _parse_item_global(hid_report_parser_global_state_t *state, unsigned char tag, unsigned long value)
{
	ASSERT(state != nullptr, KERNEL_ERROR_NULL_POINTER);

	switch(tag)
	{
		case USB_HID_ITEM_TAG_USAGE_PAGE: state->UsagePage = value; break;
		case USB_HID_ITEM_TAG_REPORT_SIZE: state->ReportSize = value;	break;
		case USB_HID_ITEM_TAG_REPORT_ID: 
			state->ReportId = value;	
			state->NextInputOffset = state->NextOutputOffset = 0; 
			break;
        case USB_HID_ITEM_TAG_REPORT_COUNT: state->ReportCount = value; break;
		case USB_HID_ITEM_TAG_MINIMUM: state->Min = value; break;
		case USB_HID_ITEM_TAG_MAXIMUM: state->Max = value; break;
	}
}

static bool _parse_item(hid_report_parser_t *parser, hid_report_parser_item_t *item)
{
	if (parser->Offset >= parser->Length)
		return false;

	usb_hid_item_t *item_hdr = (usb_hid_item_t *)(parser->Ptr + parser->Offset);
	item->Type = item_hdr->Type;

	unsigned char *data;
	if (item_hdr->Tag != 0b1111)
	{
		item->Length = _short_item_length[item_hdr->Size];
		item->Tag = item_hdr->Tag;
		data = parser->Ptr + parser->Offset + 1;
		parser->Offset += 1 + item->Length;
	}
	else
	{
		item->Length = item_hdr->DataSize;
		item->Tag = item_hdr->LongItemTag;
		data = parser->Ptr + parser->Offset + 3;
		parser->Offset += 3 + item->Length;
	}

	switch(item->Length)
	{
		case 1:	item->Value = data[0]; break;
		case 2:	item->Value = data[0] | (data[1] << 8); break;
		case 3:	item->Value = data[0] | (data[1] << 8) | (data[2] << 16); break;
		case 4:	item->Value = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24); break;
		default:	item->Data = data;	break;
	}
	return true;
}

bool usb_hid_parse_report_descriptor(hid_report_parser_t *parser, hid_report_parser_item_t *item, hid_parse_found_t *pfound)
{
	hid_report_parser_global_state_t *state = parser->Global;
	ASSERT(state != nullptr, KERNEL_ERROR_NULL_POINTER);

	bool found = false;
	while(_parse_item(parser, item))
	{
		switch(item->Type)
		{
			case USB_HID_ITEM_TYPE_GLOBAL:
				_parse_item_global(parser->Global, item->Tag, item->Value);
				break;
			case USB_HID_ITEM_TYPE_LOCAL:
				*pfound = HID_PARSE_FOUND_LOCAL;
				found = true;
				break;
			case USB_HID_ITEM_TYPE_MAIN:
				switch(item->Tag)
				{
					case USB_HID_ITEM_TAG_INPUT:
						if (state->ReportSize != 0) 
						{
							state->InputOffset = state->NextInputOffset;
							state->NextInputOffset += state->ReportSize * state->ReportCount;
							if (!(item->Value & USB_HID_INPUT_CONSTANT))
							{
								verbose(VERBOSE_DEBUG, "usb-hid", "input report #%d (offset %d, %dx%d bit) page 0x%02x", 
									state->ReportId, state->InputOffset, state->ReportCount, state->ReportSize, state->UsagePage);

								*pfound = HID_PARSE_FOUND_INPUT;
								found = true;
							}
						}
						break;
					case USB_HID_ITEM_TAG_OUTPUT:
						if (state->ReportSize != 0) 
						{
							state->OutputOffset = state->NextOutputOffset;
							state->NextOutputOffset += state->ReportSize * state->ReportCount;
							verbose(VERBOSE_DEBUG, "usb-hid", "output report #%d (offset %d, %dx%d bit) page 0x%02x", 
								state->ReportId, state->OutputOffset, state->ReportCount, state->ReportSize, state->UsagePage);

							*pfound = HID_PARSE_FOUND_OUTPUT;
							found = true;
						}
						break;
					case USB_HID_ITEM_TAG_END_COLLECTION:
						*pfound = HID_PARSE_FOUND_END;
						found = true;
						break;
				}
				break;
		}
		if (found)
			break;
	}
	return found;
}

bool usb_hid_parse_find_collection(hid_report_parser_t *parser, unsigned char collection_type)
{
	hid_report_parser_t copy = *parser;
	hid_report_parser_item_t item;
	while(_parse_item(&copy, &item))
	{
		if (item.Type == USB_HID_ITEM_TYPE_MAIN &&
			item.Tag == USB_HID_ITEM_TAG_COLLECTION && 
			item.Value == collection_type)
		{
			*parser = copy;
			return true;
		}
	}
	return false;
}

void usb_hid_parse_local_item(const hid_report_parser_item_t *item, usb_hid_desktop_usage_t *pusage, usb_hid_desktop_usage_t *pusage_min, usb_hid_desktop_usage_t *pusage_max)
{
	ASSERT(item != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(item->Type == USB_HID_ITEM_TYPE_LOCAL, KERNEL_ERROR_KERNEL_PANIC);
	switch(item->Tag)
	{
		case USB_HID_ITEM_TAG_USAGE:		*pusage = item->Value;	break;
		case USB_HID_ITEM_TAG_USAGE_MIN:	*pusage_min = item->Value;	break;
		case USB_HID_ITEM_TAG_USAGE_MAX:	*pusage_max = item->Value;	break;
	}
}

static hid_function_handler_t *_check_hid_descriptor_report(usb_host_device_t *device, usb_configuration_descriptor_t *conf_desc, usb_interface_descriptor_t *if_desc, 
	unsigned char *report_desc, unsigned desc_length)
{
	hid_function_handler_t *handler = nullptr;

	exos_mutex_lock(&_manager_lock);	
	FOREACH(node, &_manager_list)
	{
		hid_report_parser_global_state_t state = (hid_report_parser_global_state_t) { 
			.ReportId = 0 };	// FIXME
		hid_report_parser_t parser = (hid_report_parser_t) {
			.Ptr = report_desc, .Length = desc_length, .Global = &state };

		hid_driver_node_t *driver_node = (hid_driver_node_t *)node;
		const hid_driver_t *driver = driver_node->Driver;
		//ASSERT(driver != nullptr, KERNEL_ERROR_NULL_POINTER);
		//ASSERT(driver->MatchDevice != nullptr, KERNEL_ERROR_NULL_POINTER);

		//handler = driver->MatchDevice(device, conf_desc, if_desc, (report_desc != NULL && desc_length != 0) ? &parser : NULL);
		//if (handler != nullptr)
		//{
		//	verbose(VERBOSE_COMMENT, "usb-hid", "handler found; max report id = %d", handler->MaxReportId); 
		//	handler->DriverNode = driver_node;
		//	break;
		//}
	}
	exos_mutex_unlock(&_manager_lock);
	return handler;
}

static usb_host_function_t *_check_interface(usb_host_device_t *device, usb_configuration_descriptor_t *conf_desc, usb_descriptor_header_t *fn_desc)
{
	hid_function_t *func = nullptr;

	if (fn_desc->DescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE)
	{
		usb_interface_descriptor_t *if_desc = (usb_interface_descriptor_t *)fn_desc;

		if (if_desc->InterfaceClass == USB_CLASS_HID)
		{
			verbose(VERBOSE_COMMENT, "usb-hid", "interface is hid class");
			usb_hid_descriptor_t *hid_desc = (usb_hid_descriptor_t *)usb_enumerate_find_class_descriptor(
				conf_desc, if_desc,
				USB_HID_DESCRIPTOR_HID, 0);
			
			// NOTE: ensure that Hdd descriptor exists and has at elast one report descriptor
			if (hid_desc != nullptr && hid_desc->NumDescriptors >= 1 &&
				hid_desc->ReportDescriptors[0].DescriptorType == USB_HID_DESCRIPTOR_REPORT)
			{
				hid_function_handler_t *handler = NULL;

				exos_mutex_lock(&_manager_lock);	
				FOREACH(node, &_manager_list)
				{
					hid_driver_node_t *driver_node = (hid_driver_node_t *)node;
					const hid_driver_t *driver = driver_node->Driver;
					ASSERT(driver != nullptr, KERNEL_ERROR_NULL_POINTER);
					ASSERT(driver->MatchDevice != nullptr, KERNEL_ERROR_NULL_POINTER);

					handler = driver->MatchDevice(device, conf_desc, if_desc, hid_desc);
					if (handler != NULL)
						handler->Driver = driver;
				}

				if (handler != nullptr)
				{
					for(int i = 0; i < HID_MAX_INSTANCES; i++)
					{
						if (_function[i].Handler == nullptr)
						{
							func = &_function[i];
							func->InstanceIndex = i;
							break;
						}
					}

					if (func != nullptr)
					{
						usb_host_create_function((usb_host_function_t *)func, device, &_driver);
						func->Interface = if_desc->InterfaceNumber;
						func->InterfaceSubClass = if_desc->InterfaceSubClass;
						func->Protocol = if_desc->Protocol;

						usb_endpoint_descriptor_t *ep_desc;			
						ep_desc = usb_enumerate_find_endpoint_descriptor(conf_desc, if_desc, USB_TT_INTERRUPT, USB_DEVICE_TO_HOST, 0);
						if (ep_desc != nullptr)
						{
							usb_host_init_pipe_from_descriptor(device, &func->InputPipe, ep_desc);

							handler->Function = func;
							func->Handler = handler;
							func->ExitFlag = 0;

							verbose(VERBOSE_DEBUG, "usb-hid", "created new instance for interface #%d (port #%d)", 
								if_desc->InterfaceNumber, device->Port);

							return (usb_host_function_t *)func;
						}
						else verbose(VERBOSE_ERROR, "usb-hid", "cannot start interrupt IN ep for interface #%d (port #%d)", 
								if_desc->InterfaceNumber, device->Port);
					}
					else verbose(VERBOSE_ERROR, "usb-hid", "cannot allocate a new instance for interface #%d (port #%d)", 
							if_desc->InterfaceNumber, device->Port);
				}
				else verbose(VERBOSE_DEBUG, "usb-hid", "no handler matches interface #%d (port #%d)", 
						if_desc->InterfaceNumber, device->Port);
			}
			else verbose(VERBOSE_ERROR, "usb-hid", "cannot find Hid descriptor for interface #%d (port #%d)", 
					if_desc->InterfaceNumber, device->Port);			
		}
	}
	return nullptr;
}

static void _start(usb_host_function_t *usb_func, usb_configuration_descriptor_t *conf_desc, usb_descriptor_header_t *fn_desc)
{
	hid_function_t *func = (hid_function_t *)usb_func;
	hid_function_handler_t *handler = func->Handler;
	ASSERT(handler != nullptr, KERNEL_ERROR_KERNEL_PANIC);
	ASSERT(handler->Function == func, KERNEL_ERROR_KERNEL_PANIC);
	
	ASSERT(func->InstanceIndex < HID_MAX_INSTANCES, KERNEL_ERROR_KERNEL_PANIC);
	ASSERT(_function_busy[func->InstanceIndex] == 0, KERNEL_ERROR_KERNEL_PANIC);

	verbose(VERBOSE_ERROR, "usb-hid", "starting instance %d...", func->InstanceIndex);

	usb_hid_descriptor_t *hid_desc = (usb_hid_descriptor_t *)usb_enumerate_find_class_descriptor(conf_desc, 
		(usb_interface_descriptor_t *)fn_desc, USB_HID_DESCRIPTOR_HID, 0);
	if (hid_desc != NULL &&
		hid_desc->NumDescriptors >= 1 &&
		hid_desc->ReportDescriptors[0].DescriptorType == USB_HID_DESCRIPTOR_REPORT)
	{
		unsigned short desc_length = USB16TOH(hid_desc->ReportDescriptors[0].DescriptorLength);
		if (desc_length <= HID_MAX_REPORT_DESCRIPTOR_SIZE &&
			usb_host_read_if_descriptor(func->Device, func->Interface, 
			USB_HID_DESCRIPTOR_REPORT, 0, _report_buffer, desc_length))
		{
			// NOTE: initialize parser for driver to use in Start()
			hid_report_parser_global_state_t state = (hid_report_parser_global_state_t) { 
				.ReportId = 0 };	// FIXME
			hid_report_parser_t parser = (hid_report_parser_t) {
				.Ptr = _report_buffer, .Length = desc_length, .Global = &state };

			const hid_driver_t *driver = handler->Driver;
			ASSERT(driver != nullptr && driver->Start != nullptr, KERNEL_ERROR_NULL_POINTER);
			if (driver->Start(handler, &parser))
			{
				// NOTE: this should never fail, it's just we being paranoid
				bool done = usb_host_start_pipe(&func->InputPipe);
				ASSERT(done, KERNEL_ERROR_KERNEL_PANIC);

				usb_host_urb_create(&func->Request, &func->InputPipe);
				exos_dispatcher_create(&func->Dispatcher, nullptr, _dispatch, func);
				exos_dispatcher_add(_context, &func->Dispatcher, 0);
			}
			else 
			{
				func->Handler = NULL;
				verbose(VERBOSE_ERROR, "usb-hid", "handler didn't start! waiting detach...");
			}

			_function_busy[func->InstanceIndex] = 1;
		}
		else
		{
			if (desc_length > HID_MAX_REPORT_DESCRIPTOR_SIZE)
				verbose(VERBOSE_ERROR, "usb-hid", "desc_length (%d) > HID_MAX_REPORT_DESCRIPTOR_SIZE", desc_length);			

			verbose(VERBOSE_ERROR, "usb-hid", "cannot read report descriptor");		
		}
	}
}

static void _stop(usb_host_function_t *usb_func)
{
	hid_function_t *func = (hid_function_t *)usb_func;

	func->ExitFlag = 1;

	usb_host_stop_pipe(&func->InputPipe);
	// NOTE: all pending urbs should have end with error by now

	hid_function_handler_t *handler= func->Handler;
	if (handler != NULL)
	{
		exos_dispatcher_remove(_context, &func->Dispatcher);
		ASSERT(func->Request.Status != URB_STATUS_ISSUED, KERNEL_ERROR_KERNEL_PANIC);
		const hid_driver_t *driver = handler->Driver;
		ASSERT(driver != NULL && driver->Stop != NULL, KERNEL_ERROR_NULL_POINTER);
		driver->Stop(handler);

		_function->Handler = NULL;
	}

	_function_busy[func->InstanceIndex] = 0;

    verbose(VERBOSE_DEBUG, "usb-hid", "stopped instance %d", func->InstanceIndex);
}

static void _dispatch(dispatcher_context_t *context, dispatcher_t *dispatcher)
{
	hid_function_t *func = (hid_function_t *)dispatcher->CallbackState;
	ASSERT(func != nullptr, KERNEL_ERROR_NULL_POINTER);
	hid_function_handler_t *handler = func->Handler;
	ASSERT(handler != nullptr, KERNEL_ERROR_NULL_POINTER);
	const hid_driver_t *driver = handler->Driver;
	ASSERT(driver != nullptr, KERNEL_ERROR_NULL_POINTER);

	if (dispatcher->State == DISPATCHER_TIMEOUT)
	{
		verbose(VERBOSE_ERROR, "usb-hid", "TIMEOUT (%d)", func->InstanceIndex);
		exos_dispatcher_add(context, dispatcher, EXOS_TIMEOUT_NEVER);
		return;
	}

	unsigned report_bytes = func->InputPipe.MaxPacketSize;
	usb_request_buffer_t *urb = &func->Request;
	if (urb->Status != URB_STATUS_EMPTY)
	{
		int done = usb_host_end_transfer(&func->Request, EXOS_TIMEOUT_NEVER);
		if (done >= 0)
		{
			unsigned char buffer[64];
			unsigned char *data;
			unsigned char report_id;
			if (handler->MaxReportId != 0)
			{
				report_id = func->InputBuffer[0];
				data = &func->InputBuffer[1];
			}
			else
			{
				report_id = 0;
				data = func->InputBuffer;
			}

			driver->Notify(handler, report_id, data, done);
		}
		else 
		{
			verbose(VERBOSE_ERROR, "usb-hid", "report input failed");
			// TODO: recover from failure state
			func->ExitFlag = 2;
		}
	}

	if (!func->ExitFlag)
	{
		usb_host_urb_create(urb, &func->InputPipe);
		if (usb_host_begin_transfer(urb, func->InputBuffer, report_bytes))
		{
			dispatcher->Event = &urb->Event;
#ifdef HID_TIMEOUT
			exos_dispatcher_add(context, dispatcher, HID_TIMEOUT);
#else
			exos_dispatcher_add(context, dispatcher, EXOS_TIMEOUT_NEVER);
#endif
		}
		else func->ExitFlag = 3;
	}

	if (func->ExitFlag)
	{
		verbose(VERBOSE_DEBUG, "usb-hid", "stopping handler (%d)", func->InstanceIndex);
	}
}

bool usb_hid_set_idle(hid_function_t *func, unsigned char report_id, unsigned char idle)
{
	ASSERT(func != nullptr, KERNEL_ERROR_NULL_POINTER);
	usb_request_t req = (usb_request_t) {
		.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_CLASS | USB_REQTYPE_RECIPIENT_INTERFACE,
		.RequestCode = USB_HID_REQUEST_SET_IDLE,
		.Value = (idle << 8) | report_id, .Index = func->Interface, .Length = 0 };
	bool done = usb_host_ctrl_setup(func->Device, &req, nullptr, 0);
	return done;
}

bool usb_hid_set_report(hid_function_t *func, unsigned char report_type, unsigned char report_id, void *data, int length)
{
	ASSERT(func != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(data != nullptr || length == 0, KERNEL_ERROR_KERNEL_PANIC);

	usb_request_t req = (usb_request_t) {
		.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_CLASS | USB_REQTYPE_RECIPIENT_INTERFACE,
		.RequestCode = USB_HID_REQUEST_SET_REPORT,
		.Value = (report_type << 8) | report_id, .Index = func->Interface, .Length = length };
	memcpy(func->OutputBuffer, data, length);
	bool done = usb_host_ctrl_setup(func->Device, &req, func->OutputBuffer, length);
	return done;
}

unsigned usb_hid_read_field(unsigned bit_offset, unsigned bit_length, const unsigned char *report, unsigned char *data)
{
	unsigned done = 0;
	if (bit_length != 0)
	{
		unsigned offset = bit_offset >> 3;
		unsigned skip = bit_offset & 0x7;
		unsigned acc = report[offset++];
		while(bit_length > (8 - skip))
		{
			acc |= (report[offset++] << 8);
			data[done++] = acc >> skip;
			if (bit_length < 8)
				break;
			acc >>= 8;
			bit_length -= 8;
		}
		if (bit_length != 0)
			data[done++] = (acc >> skip) & ((1 << bit_length) - 1);
	}
	return done;
}




