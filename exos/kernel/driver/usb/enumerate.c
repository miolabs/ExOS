#include "enumerate.h"
#include <kernel/verbose.h>

usb_endpoint_descriptor_t *usb_enumerate_find_endpoint_descriptor(usb_configuration_descriptor_t *conf_desc, usb_interface_descriptor_t *if_desc, 
	usb_transfer_type_t ep_type, usb_direction_t ep_dir, int index)
{
	void *buffer = (void *)if_desc + if_desc->Header.Length;
	void *end = (void *)conf_desc + USB16TOH(conf_desc->TotalLength);
	
	usb_endpoint_descriptor_t *ep_desc = nullptr;
	if (buffer > (void *)conf_desc)
	{
		while (buffer < end)
		{
			usb_descriptor_header_t *desc = (usb_descriptor_header_t *)buffer;
			if (desc->DescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE) 
				break;
			if (desc->DescriptorType == USB_DESCRIPTOR_TYPE_ENDPOINT)
			{
				ep_desc = (usb_endpoint_descriptor_t *)desc;
				usb_direction_t dir = ep_desc->AddressBits.Input ?
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
	return nullptr;
}

usb_descriptor_header_t *usb_enumerate_find_class_descriptor(usb_configuration_descriptor_t *conf_desc, usb_interface_descriptor_t *if_desc,
	unsigned char descriptor_type, unsigned char index)
{
	void *buffer = (void *)if_desc + if_desc->Header.Length;
	void *end = (void *)conf_desc + USB16TOH(conf_desc->TotalLength);
	
	usb_endpoint_descriptor_t *ep_desc = nullptr;
	if (buffer > (void *)conf_desc)
	{
		while (buffer < end)
		{
			usb_descriptor_header_t *desc = (usb_descriptor_header_t *)buffer;
			if (desc->DescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE) 
				break;
			if (desc->DescriptorType == descriptor_type)
			{
				if (index != 0) index--;	// skip coincidence
				else return desc;
			}
			buffer += desc->Length;
		}
	}
	return nullptr;
}

static bool _check_interface(usb_host_function_driver_t *driver, void *arg)
{
	usb_host_enum_data_t *enum_data = (usb_host_enum_data_t *)arg;
	usb_host_device_t *device = enum_data->Device;

	usb_host_function_t *func = driver->CheckInterface(device, enum_data->ConfDesc, enum_data->FnDesc);
	if (func != nullptr) 
	{
		enum_data->Function = func;
		return true;
	}
	return false;
}

static bool _set_configuration(usb_host_device_t *device, usb_configuration_descriptor_t *conf_desc)
{
	usb_request_t req = (usb_request_t) {
		.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_RECIPIENT_DEVICE,
		.RequestCode = USB_REQUEST_SET_CONFIGURATION,
		.Value = conf_desc->ConfigurationValue, .Index = 0, .Length = 0 };
	return usb_host_ctrl_setup(device, &req, nullptr, 0);
}

static unsigned _enum_interfaces(usb_host_device_t *device, usb_configuration_descriptor_t *conf_desc)
{
	void *buffer = (void *)conf_desc;
	unsigned offset = conf_desc->Header.Length;	// NOTE: skip configuration descriptor
	unsigned done = 0;
	while (offset < USB16TOH(conf_desc->TotalLength))
	{
		usb_descriptor_header_t *fn_desc = (usb_descriptor_header_t *)(buffer + offset);
		usb_host_enum_data_t enum_data = (usb_host_enum_data_t) {
			.Device = device, .ConfDesc = conf_desc, .FnDesc = fn_desc };
	
		bool found_driver = false;
		if (fn_desc->DescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE)
		{
			usb_interface_descriptor_t *if_desc = (usb_interface_descriptor_t *)fn_desc;
			verbose(VERBOSE_DEBUG, "usbh-enum", "checking conf #%d, if #%d (alt=%d, class=$%x/$%x, proto=%d) ...", 
				conf_desc->ConfigurationIndex, if_desc->InterfaceNumber,
				if_desc->AlternateSetting, if_desc->InterfaceClass, if_desc->InterfaceSubClass, if_desc->Protocol);
			found_driver = usb_host_driver_enumerate(_check_interface, &enum_data);
		}
		else if (fn_desc->DescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION)
		{
			usb_if_association_descriptor_t *iad_desc = (usb_if_association_descriptor_t *)fn_desc;
			verbose(VERBOSE_DEBUG, "usbh-enum", "checking conf #%d, if asoc #%d (%d if)...", 
				conf_desc->ConfigurationIndex, iad_desc->FirstInterface, iad_desc->InterfaceCount);
			found_driver = usb_host_driver_enumerate(_check_interface, &enum_data);
		}

		if (found_driver)
		{
			if (done == 0)
			{
				if (!_set_configuration(device, conf_desc))
				{
					verbose(VERBOSE_DEBUG, "usbh-enum", "set configuration #%d failed!", conf_desc->ConfigurationIndex);
					break;
				}
			}

			usb_host_function_t *func = enum_data.Function;
			const usb_host_function_driver_t *driver = func->Driver;
			driver->Start(func, conf_desc, fn_desc);
			list_add_tail(&device->Functions, &func->Node); 
			done++;
		}

		offset += fn_desc->Length;
	}
	return done;
}

#define USB_ENUM_BUFFER_SIZE 2048
static unsigned char _buffer[USB_ENUM_BUFFER_SIZE] __usb;

static bool _parse_configuration(usb_host_device_t *device, unsigned char conf_index)
{
	bool done = usb_host_read_device_descriptor(device, USB_DESCRIPTOR_TYPE_CONFIGURATION, conf_index,
		_buffer, sizeof(usb_configuration_descriptor_t));
	if (done)
	{
		usb_configuration_descriptor_t *conf_desc = (usb_configuration_descriptor_t *)_buffer;
		
		verbose(VERBOSE_DEBUG, "usbh-enum", "checking conf[%d] (#%d, val=%02xh)...", conf_index,
			conf_desc->ConfigurationIndex, conf_desc->ConfigurationValue);

		unsigned total_length = USB16TOH(conf_desc->TotalLength);
		if (total_length <= USB_ENUM_BUFFER_SIZE) 
		{
			done = usb_host_read_device_descriptor(device, USB_DESCRIPTOR_TYPE_CONFIGURATION, conf_index,
				_buffer, total_length);
			if (done)
			{
				done = (_enum_interfaces(device, conf_desc) != 0);
				if (!done) verbose(VERBOSE_DEBUG, "usbh-enum", "no driver found for conf[%d]", conf_index);
			}
			else verbose(VERBOSE_DEBUG, "usbh-enum", "cannot read conf[%d] descriptor (full)", conf_index); 
		}
		else verbose(VERBOSE_DEBUG, "usbh-enum", "conf[%d] descriptor is too big!", conf_index);
	}
	else verbose(VERBOSE_DEBUG, "usbh-enum", "cannot read conf[%d] descriptor (short)", conf_index);
	
	return done;
}

bool usb_host_enumerate(usb_host_device_t *device, usb_device_descriptor_t *dev_desc)
{
	// extract device data
	device->Vendor = USB16TOH(dev_desc->VendorId);
	device->Product = USB16TOH(dev_desc->ProductId);
	verbose(VERBOSE_DEBUG, "usbh-enum", "device %04x/%04x", device->Vendor, device->Product);

	bool done = false;
	unsigned char num_configs = dev_desc->NumConfigurations;
	for (unsigned char conf = 0; conf < num_configs; conf++)
	{
		if (_parse_configuration(device, conf))
		{
			done = true;
			break;
		}
	}
	return done;
}

 

