#include "enumerate.h"


USB_ENDPOINT_DESCRIPTOR *usb_enumerate_find_endpoint_descriptor(USB_CONFIGURATION_DESCRIPTOR *conf_desc, USB_INTERFACE_DESCRIPTOR *if_desc, 
	USB_TRANSFERTYPE ep_type, USB_DIRECTION ep_dir, int index)
{
	void *buffer = (void *)if_desc + if_desc->Header.Length;
	void *end = (void *)conf_desc + USB16TOH(conf_desc->TotalLength);
	
	USB_ENDPOINT_DESCRIPTOR *ep_desc = NULL;
	if (buffer > (void *)conf_desc)
	{
		while (buffer < end)
		{
			USB_DESCRIPTOR_HEADER *desc = (USB_DESCRIPTOR_HEADER *)buffer;
			if (desc->DescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE) break;
			if (desc->DescriptorType == USB_DESCRIPTOR_TYPE_ENDPOINT)
			{
				ep_desc = (USB_ENDPOINT_DESCRIPTOR *)desc;
				USB_DIRECTION dir = ep_desc->AddressBits.Input ?
					USB_DEVICE_TO_HOST : USB_HOST_TO_DEVICE;
				if (ep_desc->AttributesBits.TransferType == ep_type &&
					dir == ep_dir)
				{
					if (index != 0) index--;	// skip coincidence
					else return ep_desc;
				}
			}
			buffer += desc->Length;
		}
	}
	return NULL;
}


static int _check_interface(USB_HOST_FUNCTION_DRIVER *driver, void *arg)
{
	USB_HOST_ENUM_DATA *enum_data = (USB_HOST_ENUM_DATA *)arg;
	USB_HOST_DEVICE *device = enum_data->Device;

	USB_HOST_FUNCTION *func = driver->CheckInterface(device, enum_data->ConfDesc, enum_data->FnDesc);
	if (func != NULL) 
	{
		enum_data->Function = func;
		return 1;
	}
	return 0;
}

static int _set_configuration(USB_HOST_DEVICE *device, USB_CONFIGURATION_DESCRIPTOR *conf_desc)
{
	USB_REQUEST req = (USB_REQUEST) {
		.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_RECIPIENT_DEVICE,
		.RequestCode = USB_REQUEST_SET_CONFIGURATION,
		.Value = conf_desc->ConfigurationValue, .Index = 0, .Length = 0 };
	return usb_host_ctrl_setup(device, &req, NULL, 0);
}

static int _enum_interfaces(USB_HOST_DEVICE *device, USB_CONFIGURATION_DESCRIPTOR *conf_desc)
{
	void *buffer = (void *)conf_desc;
	int offset = conf_desc->Header.Length;	// skip itself conf. descriptor
	int rem_interfaces = conf_desc->NumInterfaces;
	int done = 0;
	while (offset < USB16TOH(conf_desc->TotalLength)) // && rem_interfaces > 0)
	{
		USB_DESCRIPTOR_HEADER *fn_desc = (USB_DESCRIPTOR_HEADER *)(buffer + offset);
		USB_HOST_ENUM_DATA enum_data = (USB_HOST_ENUM_DATA) {
			.Device = (USB_HOST_DEVICE *)device, .ConfDesc = conf_desc, .FnDesc = fn_desc };
	
		int found_driver = 0;
		if (fn_desc->DescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE)
		{
			USB_INTERFACE_DESCRIPTOR *if_desc = (USB_INTERFACE_DESCRIPTOR *)fn_desc;
			found_driver = usb_host_driver_enumerate(_check_interface, &enum_data);
			rem_interfaces--;
		}
		else if (fn_desc->DescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION)
		{
			USB_INTERFACE_ASSOCIATION_DESCRIPTOR *iad_desc = (USB_INTERFACE_ASSOCIATION_DESCRIPTOR *)fn_desc;
			found_driver = usb_host_driver_enumerate(_check_interface, &enum_data);
		}

		if (found_driver)
		{
			if (done == 0)
			{
				if (!_set_configuration(device, conf_desc))
					break;
			}
			USB_HOST_FUNCTION *func = enum_data.Function;
			const USB_HOST_FUNCTION_DRIVER *driver = func->Driver;
			driver->Start(func);
			list_add_tail(&device->Functions, (EXOS_NODE *)func); 
			done++;
		}

		offset += fn_desc->Length;
	}
	return done;
}

#define USB_ENUM_BUFFER_SIZE 2048
static unsigned char _buffer[USB_ENUM_BUFFER_SIZE] __usb;

static int _parse_configuration(USB_HOST_DEVICE *device, int conf_index)
{
	int done = usb_host_read_descriptor(device, USB_DESCRIPTOR_TYPE_CONFIGURATION, conf_index,
		_buffer, sizeof(USB_CONFIGURATION_DESCRIPTOR));
	if (done)
	{
		USB_CONFIGURATION_DESCRIPTOR *conf_desc = (USB_CONFIGURATION_DESCRIPTOR *)_buffer;
		
		int total_length = USB16TOH(conf_desc->TotalLength);
		if (total_length <= USB_ENUM_BUFFER_SIZE) 
		{
			done = usb_host_read_descriptor(device, USB_DESCRIPTOR_TYPE_CONFIGURATION, conf_index,
				_buffer, total_length);
			if (done)
			{
				done = _enum_interfaces(device, conf_desc);
			}
		}
	}
	return done;
}

int usb_host_enumerate(USB_HOST_DEVICE *device, USB_DEVICE_DESCRIPTOR *dev_desc)
{
	// extract device data
	device->Vendor = USB16TOH(dev_desc->VendorId);
	device->Product = USB16TOH(dev_desc->ProductId);

	int done = 0;
	int num_configs = dev_desc->NumConfigurations;
	for (int conf = 0; conf < num_configs; conf++)
	{
		if (_parse_configuration(device, conf))
		{
			done = 1;
			break;
		}
	}
	return done;
}


