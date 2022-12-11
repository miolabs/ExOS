#ifndef USB_DEVICE_CONFIG_H
#define USB_DEVICE_CONFIG_H

#include <usb/usb.h>
#include <usb/device.h>

typedef struct
{
	unsigned (*FillConfigurationDescriptor)(unsigned conf_index, usb_configuration_descriptor_t *conf_desc, unsigned buffer_size);
	void (*Configured)(unsigned conf_index, bool configured);
	bool (*DeviceClassRequest)(usb_request_t *req, void **pdata, int *plength);
	bool (*DeviceVendorRequest)(usb_request_t *req, void **pdata, int *plength);
} usb_device_configuration_driver_t;

typedef struct
{
	node_t Node;
	const usb_device_configuration_driver_t *Driver;
	list_t Interfaces;
	unsigned char Value;
	unsigned char Index;
} usb_device_configuration_t;

#define USB_CONFIG_POWER(ma) (ma / 2)
#define USB_EP_IN(ep) (ep | 0x80)
#define USB_EP_OUT(ep) (ep | 0x00)


//prototypes
void usb_device_config_initialize();
void usb_device_config_create(usb_device_configuration_t *conf, unsigned char value, const usb_device_configuration_driver_t *driver);
void usb_device_config_add_interface(usb_device_configuration_t *conf, usb_device_interface_t *iface, const void *instance_data);
void usb_device_config_register(usb_device_configuration_t *conf);
usb_device_configuration_t *usb_device_config_get(unsigned index);
unsigned usb_device_config_get_count();
void usb_device_interface_create(usb_device_interface_t *iface, const usb_device_interface_driver_t *driver);

unsigned usb_device_config_add_string(usb_device_string_t *str, const char *value);
const char *usb_device_config_get_string(int index);

usb_descriptor_header_t *usb_device_config_get_descriptor(unsigned short req_value, unsigned short req_index, unsigned *plength);

// optional customization callbacks
void usb_device_add_configurations() __weak;
int usb_device_fill_device_descriptor(usb_device_descriptor_t *desc);
int usb_device_fill_class_descriptor(usb_descriptor_header_t *desc, unsigned short req_value, unsigned short req_index, int max_plength);
int usb_device_fill_vendor_descriptor(usb_descriptor_header_t *desc, unsigned short req_value, unsigned short req_index, int max_plength);


#endif // USB_DEVICE_CONFIG_H


