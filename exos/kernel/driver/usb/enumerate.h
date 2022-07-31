#ifndef USB_HOST_ENUMERATE_H
#define USB_HOST_ENUMERATE_H

#include <usb/host.h>

typedef struct
{
	usb_host_device_t *Device;
	usb_configuration_descriptor_t *ConfDesc;
	usb_descriptor_header_t *FnDesc;
	usb_host_function_t *Function;
} usb_host_enum_data_t;

// prototypes
bool usb_host_enumerate(usb_host_device_t *device, usb_device_descriptor_t *dev_desc);

usb_endpoint_descriptor_t *usb_enumerate_find_endpoint_descriptor(usb_configuration_descriptor_t *conf_desc, usb_interface_descriptor_t *if_desc, 
	usb_transfer_type_t ep_type, usb_direction_t ep_dir, int index);
usb_descriptor_header_t *usb_enumerate_find_class_descriptor(usb_configuration_descriptor_t *conf_desc, usb_interface_descriptor_t *if_desc,
	unsigned char descriptor_type, unsigned char index);

#endif // USB_HOST_ENUMERATE_H
