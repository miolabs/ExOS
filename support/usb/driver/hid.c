#include "hid.h"
#include <usb/enumerate.h>
#include <kernel/fifo.h>
#include <kernel/dispatch.h>
#include <kernel/panic.h>
#include <kernel/machine/hal.h>

#ifndef HID_MAX_INSTANCES
#define HID_MAX_INSTANCES 1
#endif

static USB_HOST_FUNCTION *_check_interface(USB_HOST_DEVICE *device, USB_CONFIGURATION_DESCRIPTOR *conf_desc, USB_DESCRIPTOR_HEADER *fn_desc);
static void _start(USB_HOST_FUNCTION *func);
static void _stop(USB_HOST_FUNCTION *func);

static const USB_HOST_FUNCTION_DRIVER _driver = { _check_interface, _start, _stop };
static USB_HOST_FUNCTION_DRIVER_NODE _driver_node;
static HID_FUNCTION _function[HID_MAX_INSTANCES] __usb;
static unsigned char _function_busy[HID_MAX_INSTANCES];

#define THREAD_STACK 1024
static EXOS_THREAD _thread;
static unsigned char _stack[THREAD_STACK] __attribute__((aligned(16)));
static void *_service(void *arg);
static EXOS_DISPATCHER_CONTEXT _service_context;
static void _dispatch(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher);

#ifndef HID_MAX_REPORT_DESCRIPTOR_SIZE
#define HID_MAX_REPORT_DESCRIPTOR_SIZE 128
#endif

#ifndef HID_MAX_REPORT_INPUT_COUNT
#define HID_MAX_REPORT_INPUT_COUNT 32
#endif

static HID_REPORT_INPUT _report_array[HID_MAX_REPORT_INPUT_COUNT];
static EXOS_FIFO _free_report_inputs;
static EXOS_MUTEX _manager_lock;
static EXOS_LIST _manager_list;

void usbd_hid_initialize()
{
	exos_fifo_create(&_free_report_inputs, NULL);
	for(int i = 0; i < HID_MAX_REPORT_INPUT_COUNT; i++)
		exos_fifo_queue(&_free_report_inputs, (EXOS_NODE *)&_report_array[i]);

	exos_dispatcher_context_create(&_service_context);
	exos_thread_create(&_thread, 0, _stack, THREAD_STACK, NULL, _service, &_service_context);

	exos_mutex_create(&_manager_lock);
	list_initialize(&_manager_list);
	_driver_node = (USB_HOST_FUNCTION_DRIVER_NODE) { .Driver = &_driver };
	usb_host_driver_register(&_driver_node);

}

static USB_HOST_FUNCTION *_check_interface(USB_HOST_DEVICE *device, USB_CONFIGURATION_DESCRIPTOR *conf_desc, USB_DESCRIPTOR_HEADER *fn_desc)
{
	if (fn_desc->DescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE)
	{
		USB_INTERFACE_DESCRIPTOR *if_desc = (USB_INTERFACE_DESCRIPTOR *)fn_desc;

		if (if_desc->InterfaceClass == USB_CLASS_HID)
		{
			HID_FUNCTION *func = NULL;
			for(int i = 0; i < HID_MAX_INSTANCES; i++)
			{
				if (_function_busy[i] == 0)
				{
					func = &_function[i];
					func->InstanceIndex = i;
				}
			}

			if (func != NULL)
			{
				usb_host_create_function((USB_HOST_FUNCTION *)func, device, &_driver);
				func->Interface = if_desc->InterfaceNumber;
				func->InterfaceSubClass = if_desc->InterfaceSubClass;
				func->Protocol = if_desc->Protocol;

				USB_ENDPOINT_DESCRIPTOR *ep_desc;			
				ep_desc = usb_enumerate_find_endpoint_descriptor(conf_desc, if_desc, USB_TT_INTERRUPT, USB_DEVICE_TO_HOST, 0);
				if (!ep_desc) return NULL;
				usb_host_init_pipe_from_descriptor(device, &func->InputPipe, ep_desc);

				USB_HID_DESCRIPTOR *hid_desc = (USB_HID_DESCRIPTOR *)usb_enumerate_find_class_descriptor(conf_desc, if_desc,
					USB_HID_DESCRIPTOR_HID, 0);

				func->MaxReportId = 0;
                exos_mutex_create(&func->InputLock);
				list_initialize(&func->Inputs);
				func->ExitFlag = 0;

				return (USB_HOST_FUNCTION *)func;
			}
		}
	}
	return NULL;
}

static unsigned char _report_buffer[HID_MAX_REPORT_DESCRIPTOR_SIZE] __usb;

static const unsigned char _short_item_length[4] = { 0, 1, 2, 4 };

typedef struct
{
	unsigned long Offset;

	unsigned char ReportId;
	unsigned char UsagePage;
	unsigned char Usage;
	unsigned char ReportCount;
	
	unsigned char ReportSize;
	unsigned long Min;
	unsigned long Max;
} USB_HID_ITEM_PARSE_STATE;

static int _match_handler(HID_FUNCTION_HANDLER *handler, HID_REPORT_INPUT *input)
{
#ifdef DEBUG
	if (handler->DriverNode == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	const HID_DRIVER *driver = handler->DriverNode->Driver;
#ifdef DEBUG
	if (driver == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
	if (driver->MatchInputHandler == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	return driver->MatchInputHandler(handler, input);
}

static void _parse_item_global(USB_HID_ITEM_PARSE_STATE *state, unsigned char tag, unsigned long value)
{
	switch(tag)
	{
		case USB_HID_ITEM_TAG_USAGE_PAGE: state->UsagePage = value; break;
		case USB_HID_ITEM_TAG_REPORT_SIZE: state->ReportSize = value;	break;
		case USB_HID_ITEM_TAG_REPORT_ID: state->ReportId = value;	state->Offset = 0; break;
        case USB_HID_ITEM_TAG_REPORT_COUNT: state->ReportCount = value; break;
		case USB_HID_ITEM_TAG_MINIMUM: state->Min = value; break;
		case USB_HID_ITEM_TAG_MAXIMUM: state->Max = value; break;
	}
}

static void _parse_item_local(USB_HID_ITEM_PARSE_STATE *state, unsigned char tag, unsigned long value)
{
	switch(tag)
	{
		case USB_HID_ITEM_TAG_USAGE: state->Usage = value; break;
	}
}

static void _parse_items(HID_FUNCTION_HANDLER *handler, unsigned char *ptr, int length)
{
	HID_FUNCTION *func = handler->Function;
#ifdef DEBUG
	if (func == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	unsigned long offset = 0;
	unsigned long value;
	unsigned char *data;
	USB_HID_ITEM_PARSE_STATE state = (USB_HID_ITEM_PARSE_STATE){ .ReportId = 0, .Offset = 0 };

	while(offset < length)
	{
		USB_HID_ITEM *item = (USB_HID_ITEM *)(ptr + offset);
		unsigned char length, tag;
		if (item->Tag != 0b1111)
		{
			length = _short_item_length[item->Size];
			tag = item->Tag;
			data = ptr + offset + 1;
			offset += 1 + length;
		}
		else
		{
			length = item->DataSize;
			tag = item->LongItemTag;
			data = ptr + offset + 3;
			offset += 3 + length;
		}

		switch(length)
		{
			case 1:	value = data[0]; break;
			case 2:	value = data[0] | (data[1] << 8); break;
			case 3:	value = data[0] | (data[1] << 8) | (data[2] << 16); break;
			case 4:	value = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24); break;
			default:	value = 0;	break;
		}

		switch(item->Type)
		{
			case USB_HID_ITEM_TYPE_GLOBAL:
				_parse_item_global(&state, tag, value);
				break;
			case USB_HID_ITEM_TYPE_LOCAL:
				_parse_item_local(&state, tag, value);
				break;
			case USB_HID_ITEM_TYPE_MAIN:
				switch(tag)
				{
					case USB_HID_ITEM_TAG_INPUT:
						if (state.ReportSize == 0) break;
						int count = (state.ReportCount == 0) ? 1 : state.ReportCount;
						if (!(value & USB_HID_INPUT_CONSTANT))
						{
							HID_REPORT_INPUT *input = (HID_REPORT_INPUT *)exos_fifo_dequeue(&_free_report_inputs);
							
							if (input != NULL)
							{
								input->ReportId = state.ReportId;
								input->UsagePage = state.UsagePage;
								input->Usage = state.Usage;
								input->Offset = state.Offset;
								input->Size = state.ReportSize;
								input->Count = count;
								input->Min = state.Min;
								input->Max = state.Max;
								input->InputFlags = value;

								if (_match_handler(handler, input))
								{
									//input->Handler = handler;
									list_add_tail(&func->Inputs, (EXOS_NODE *)input);
								}
								else
								{
									exos_fifo_queue(&_free_report_inputs, (EXOS_NODE *)input);
								}
							}
#ifdef DEBUG
							else
							{
								// TODO: issue runtime warning for allocation failure
							}
#endif
						}
						state.Offset += state.ReportSize * count;

						if (state.ReportId > func->MaxReportId) func->MaxReportId = state.ReportId;
						break;
				}
				break;
		}
	}
}

static void _start(USB_HOST_FUNCTION *usb_func)
{
	HID_FUNCTION *func = (HID_FUNCTION *)usb_func;
#ifdef DEBUG
	if (_function_busy[func->InstanceIndex] != 0)
		kernel_panic(KERNEL_ERROR_ALREADY_IN_USE);
#endif
	HID_FUNCTION_HANDLER *handler = NULL;
	const HID_DRIVER *driver;
	exos_mutex_lock(&_manager_lock);
	
	FOREACH(node, &_manager_list)
	{
		HID_DRIVER_NODE *driver_node = (HID_DRIVER_NODE *)node;
		driver = driver_node->Driver;
#ifdef DEBUG
		if (driver == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
		if (driver->MatchDevice != NULL)
		{
			handler = driver->MatchDevice(func);
			if (handler != NULL)
			{
				handler->DriverNode = driver_node;
				handler->Function = func;
				func->Handler = handler;
				break;
			}
		}
	}

	if (handler != NULL)
	{
		_function_busy[func->InstanceIndex] = 1;

		int done = usb_host_read_if_descriptor(func->Device, func->Interface, 
			USB_HID_DESCRIPTOR_HID, 0, func->InputBuffer, 64);
		USB_HID_DESCRIPTOR *hid_desc = (USB_HID_DESCRIPTOR *)func->InputBuffer;
	
		if (done && hid_desc->NumDescriptors >= 1 &&
			hid_desc->ClassDescriptors[0].DescriptorType == USB_HID_DESCRIPTOR_REPORT)
		{
			unsigned short length = USB16TOH(hid_desc->ClassDescriptors[0].DescriptorLength);
			if (length <= HID_MAX_REPORT_DESCRIPTOR_SIZE &&
				usb_host_read_if_descriptor(func->Device, func->Interface, 
					USB_HID_DESCRIPTOR_REPORT, 0, _report_buffer, length))
			{
				_parse_items(handler, _report_buffer, length);
			}
		}
	}

	exos_mutex_unlock(&_manager_lock);

	usb_host_start_pipe(&func->InputPipe);

	if (handler != NULL)
	{
   		driver->Start(handler);
		func->StartedFlag = func->ExitFlag = 0;
		func->Dispatcher = (EXOS_DISPATCHER) { .Callback = _dispatch, .CallbackState = func };
		exos_dispatcher_add(&_service_context, &func->Dispatcher, 0);
	}
}

static void _stop(USB_HOST_FUNCTION *usb_func)
{
	HID_FUNCTION *func = (HID_FUNCTION *)usb_func;

	func->ExitFlag = 1;
	usb_host_stop_pipe(&func->InputPipe);
	//exos_thread_join(&_thread);
	_function_busy[func->InstanceIndex] = 0;
}

static int _read_field(HID_REPORT_INPUT *input, unsigned char *report, unsigned char *data)
{
	int length = input->Size * input->Count;
	int done = 0;
	if (length != 0)
	{
		unsigned offset = input->Offset >> 3;
		unsigned shift = (8 - (input->Offset + length)) & 0x7;
		unsigned skip = input->Offset & 0x7;
		unsigned acc = report[offset++] & (0xFF >> skip);
		length -= 8 - (shift + skip);
		while(length > 0)
		{
			acc = (acc << 8) | report[offset++];
			data[done++] = acc >> (8 + shift);
			length -= 8;
		}
		data[done++] = acc >> shift;
	}
	return done;
}

static void *_service(void *arg)
{
	EXOS_DISPATCHER_CONTEXT *context = (EXOS_DISPATCHER_CONTEXT *)arg;
	while(1)
	{
		exos_dispatch(&_service_context, EXOS_TIMEOUT_NEVER);
	}
}

static void _debug()
{
}

static void _dispatch(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher)
{
	HID_FUNCTION *func = (HID_FUNCTION *)dispatcher->CallbackState;
	HID_FUNCTION_HANDLER *handler = func->Handler;
	const HID_DRIVER *driver = handler->DriverNode->Driver;
#ifdef DEBUG
	if (func == NULL || handler == NULL || handler->DriverNode == NULL || driver == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	int report_bytes = func->InputPipe.MaxPacketSize;

	if (func->StartedFlag)
	{
		int done = usb_host_end_bulk_transfer(&func->Urb, EXOS_TIMEOUT_NEVER);
		if (done >= 0)
		{
			unsigned char buffer[64];
			unsigned char *data;
			unsigned char report_id;
			if (func->MaxReportId != 0)
			{
				report_id = func->InputBuffer[0];
				data = &func->InputBuffer[1];
			}
			else
			{
				report_id = 0;
				data = func->InputBuffer;
			}

			exos_mutex_lock(&func->InputLock);
			FOREACH(node, &func->Inputs)
			{
				HID_REPORT_INPUT *input = (HID_REPORT_INPUT *)node;
				if (input->ReportId != report_id)
					continue;
	
				_read_field(input, data, buffer);
				driver->Notify(handler, input, buffer);
			}
			exos_mutex_unlock(&func->InputLock);
		}
		else 
		{
			// TODO: recover from failure state
			func->ExitFlag = 2;
		}
	}

	if (!func->ExitFlag)
	{
		usb_host_urb_create(&func->Urb, &func->InputPipe);
		if (usb_host_begin_bulk_transfer(&func->Urb, func->InputBuffer, report_bytes))
		{
			func->StartedFlag = 1;
			dispatcher->Event = &func->Urb.Event;
            exos_dispatcher_add(context, dispatcher, EXOS_TIMEOUT_NEVER);
		}
        else func->ExitFlag = 3;
	}

	if (func->ExitFlag)
	{
		driver->Stop(handler);

		exos_mutex_lock(&func->InputLock);
		HID_REPORT_INPUT *iinput = (HID_REPORT_INPUT *)func->Inputs.Head;
		while(iinput != (HID_REPORT_INPUT *)&func->Inputs.Tail)
		{
			HID_REPORT_INPUT *input = iinput;
			iinput = (HID_REPORT_INPUT *)iinput->Node.Succ;
	
			list_remove((EXOS_NODE *)input);
			exos_fifo_queue(&_free_report_inputs, (EXOS_NODE *)input);
		}
		exos_mutex_unlock(&func->InputLock);
	}
}

int usbd_hid_add_driver(HID_DRIVER_NODE *node)
{
	exos_mutex_lock(&_manager_lock);
	list_add_tail(&_manager_list, (EXOS_NODE *)node);
	exos_mutex_unlock(&_manager_lock);
	return 1;
}

int usbd_hid_set_report(HID_FUNCTION *func, unsigned char report_type, unsigned char report_id, void *data, int length)
{
	USB_REQUEST req = (USB_REQUEST) {
		.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_CLASS | USB_REQTYPE_RECIPIENT_INTERFACE,
		.RequestCode = HID_REQUEST_SET_REPORT,
		.Value = (report_type << 8) | report_id, .Index = func->Interface, .Length = length };
	__mem_copy(func->OutputBuffer, func->OutputBuffer + length, data);
	int done = usb_host_ctrl_setup(func->Device, &req, func->OutputBuffer, length);
	return done;
}






