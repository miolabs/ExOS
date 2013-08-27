#include "hid.h"
#include <usb/enumerate.h>
#include <kernel/fifo.h>
#include <kernel/panic.h>
#include <kernel/machine/hal.h>

static USB_HOST_FUNCTION *_check_interface(USB_HOST_DEVICE *device, USB_CONFIGURATION_DESCRIPTOR *conf_desc, USB_DESCRIPTOR_HEADER *fn_desc);
static void _start(USB_HOST_FUNCTION *func);
static void _stop(USB_HOST_FUNCTION *func);

static const USB_HOST_FUNCTION_DRIVER _driver = { _check_interface, _start, _stop };
static USB_HOST_FUNCTION_DRIVER_NODE _driver_node;
static HID_FUNCTION _function __usb;	// currently, a single instance is supported
static int _function_busy = 0;

#define THREAD_STACK 1024
static EXOS_THREAD _thread;
static unsigned char _stack[THREAD_STACK] __attribute__((aligned(16)));
static void *_service(void *arg);

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
			HID_FUNCTION *func = _function_busy ? NULL : &_function;

			if (func != NULL)
			{
				usb_host_create_function((USB_HOST_FUNCTION *)func, device, &_driver);
				func->Interface = if_desc->InterfaceNumber;
	
				USB_ENDPOINT_DESCRIPTOR *ep_desc;			
				ep_desc = usb_enumerate_find_endpoint_descriptor(conf_desc, if_desc, USB_TT_INTERRUPT, USB_DEVICE_TO_HOST, 0);
				if (!ep_desc) return NULL;
				usb_host_init_pipe_from_descriptor(device, &func->InputPipe, ep_desc);

				USB_HID_DESCRIPTOR *hid_desc = (USB_HID_DESCRIPTOR *)usb_enumerate_find_class_descriptor(conf_desc, if_desc,
					USB_HID_DESCRIPTOR_HID, 0);

				func->MaxReportId = 0;
                exos_mutex_create(&func->InputLock);
				list_initialize(&func->Inputs);

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

static HID_REPORT_MANAGER *_match_handler(HID_FUNCTION *func, HID_REPORT_INPUT *input)
{
	HID_REPORT_MANAGER *matching = NULL;
	exos_mutex_lock(&_manager_lock);
	FOREACH(node, &_manager_list)
	{
		HID_REPORT_MANAGER *manager = (HID_REPORT_MANAGER *)node;
		const HID_REPORT_DRIVER *driver = manager->Driver;
#ifdef DEBUG
		if (driver == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
		if (driver->MatchDevice != NULL && !driver->MatchDevice(func->Device)) continue;
		if (driver->MatchInputHandler)
		{
			if (driver->MatchInputHandler(func, input))
			{
				matching = manager;
				break;
			}
		}
	}
	exos_mutex_unlock(&_manager_lock);
	return matching;
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

static void _parse_items(HID_FUNCTION *func, unsigned char *ptr, int length)
{
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

								HID_REPORT_MANAGER *manager = _match_handler(func, input);
								if (manager != NULL)
								{
									input->Manager = manager;
									list_add_tail(&func->Inputs, (EXOS_NODE *)input);
								}
								else
								{
									exos_fifo_queue(&_free_report_inputs, (EXOS_NODE *)input);
								}
							}
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
			_parse_items(func, _report_buffer, length);
		}
	}

	usb_host_start_pipe(&func->InputPipe);

	exos_thread_create(&_thread, 0, _stack, THREAD_STACK, NULL, _service, func);
}

static void _stop(USB_HOST_FUNCTION *usb_func)
{
	HID_FUNCTION *func = (HID_FUNCTION *)usb_func;

	// TODO: stop thread:
	// setup a non zero timeout in thread loop -> urb timeout support 
	// check urb termination (due to timeout) with device detached
	// exit thread with termination flag/state

	usb_host_stop_pipe(&func->InputPipe);

	// TODO
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
	HID_FUNCTION *func = (HID_FUNCTION *)arg;

	exos_mutex_lock(&_manager_lock);
	FOREACH(node, &_manager_list)
	{
		HID_REPORT_MANAGER *manager = (HID_REPORT_MANAGER *)node;
		const HID_REPORT_DRIVER *driver = manager->Driver;
#ifdef DEBUG
		if (driver == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
		if (driver->EndReportEnum != NULL)
			driver->EndReportEnum(func);
	}
	exos_mutex_unlock(&_manager_lock);

	int report_bytes = func->InputPipe.MaxPacketSize;
	USB_REQUEST_BUFFER urb;
	while(1)
	{
		usb_host_urb_create(&urb, &func->InputPipe);
		// NOTE: it's not a bulk transfer, but processing is compatible using an int pipe
		usb_host_begin_bulk_transfer(&urb, func->InputBuffer, report_bytes);
		usb_host_end_bulk_transfer(&urb, EXOS_TIMEOUT_NEVER);

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
			HID_REPORT_MANAGER *manager = input->Manager;
			if (manager != NULL)
			{
				if (input->ReportId != report_id)
					continue;

				_read_field(input, data, buffer);
				const HID_REPORT_DRIVER *driver = manager->Driver;
				driver->Notify(func, input, buffer);
			}
		}
		exos_mutex_unlock(&func->InputLock);
	}
}

int usbd_hid_add_manager(HID_REPORT_MANAGER *manager)
{
	exos_mutex_lock(&_manager_lock);
	list_add_tail(&_manager_list, (EXOS_NODE *)manager);
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






