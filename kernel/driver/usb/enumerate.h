#ifndef USB_HOST_ENUMERATE_H
#define USB_HOST_ENUMERATE_H

#include <usb/host.h>

typedef struct
{
	USB_HOST_DEVICE *Device;
	USB_CONFIGURATION_DESCRIPTOR *ConfDesc;
	USB_DESCRIPTOR_HEADER *FnDesc;
	USB_HOST_FUNCTION *Function;
} USB_HOST_ENUM_DATA;

// prototypes
int usb_host_enumerate(USB_HOST_DEVICE *device, USB_DEVICE_DESCRIPTOR *dev_desc);

USB_ENDPOINT_DESCRIPTOR *usb_enumerate_find_endpoint_descriptor(USB_CONFIGURATION_DESCRIPTOR *conf_desc, USB_INTERFACE_DESCRIPTOR *if_desc, 
	USB_TRANSFERTYPE ep_type, USB_DIRECTION ep_dir, int index);
USB_DESCRIPTOR_HEADER *usb_enumerate_find_class_descriptor(USB_CONFIGURATION_DESCRIPTOR *conf_desc, USB_INTERFACE_DESCRIPTOR *if_desc,
	unsigned char descriptor_type, unsigned char index);

#endif // USB_HOST_ENUMERATE_H
