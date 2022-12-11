#include "configuration.h"
#include "device.h"
#include <kernel/mutex.h>
#include <kernel/panic.h>
#include <kernel/verbose.h>

static list_t _configs;
static mutex_t _configs_lock;
static list_t _strings;
static mutex_t _strings_lock;
static unsigned char _manufacturer_str = 0;
static unsigned char _product_str = 0;
static unsigned char _serialnumber_str = 0;

#ifndef USB_CONF_BUFFER_SIZE
#define USB_CONF_BUFFER_SIZE 255
#endif
static unsigned char _desc_buffer[USB_CONF_BUFFER_SIZE] __usb __attribute__((__aligned__((8))));

void usb_device_config_initialize()
{
	list_initialize(&_configs);
	exos_mutex_create(&_configs_lock);
	list_initialize(&_strings);
	exos_mutex_create(&_strings_lock);

#ifdef USB_USER_DEVICE_MANUFACTURER
	static usb_device_string_t _manufacturer_node;
	_manufacturer_str = usb_device_config_add_string(&_manufacturer_node, USB_USER_DEVICE_MANUFACTURER);
#endif
#ifdef USB_USER_DEVICE_PRODUCT
	static usb_device_string_t _product_node;
	_product_str = usb_device_config_add_string(&_product_node, USB_USER_DEVICE_PRODUCT);
#endif
#ifdef USB_USER_DEVICE_SERIALNUMBER
	static usb_device_string_t _serialnumber_node;
	_serialnumber_str = usb_device_config_add_string(&_serialnumber_node, USB_USER_DEVICE_SERIALNUMBER);
#endif

	usb_device_add_configurations();

	// NOTE: check that we got at least one configuration
	ASSERT(NULL != usb_device_config_get(0), KERNEL_ERROR_KERNEL_PANIC);
}

void usb_device_config_create(usb_device_configuration_t *conf, unsigned char value, const usb_device_configuration_driver_t *driver)
{
	conf->Driver = driver;
	conf->Value = value;
	conf->Index = -1;
	list_initialize(&conf->Interfaces);
}

void usb_device_config_register(usb_device_configuration_t *conf)
{
	exos_mutex_lock(&_configs_lock);

	if (conf == nullptr || conf->Driver == nullptr)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
	if (list_find_node(&_configs, (node_t *)conf))
		kernel_panic(KERNEL_ERROR_LIST_ALREADY_CONTAINS_NODE);

	conf->Index = list_get_count(&_configs);

	list_add_tail(&_configs, (node_t *)conf);
	exos_mutex_unlock(&_configs_lock);
}

usb_device_configuration_t *usb_device_config_get(unsigned index)
{
	usb_device_configuration_t *conf = nullptr;
	exos_mutex_lock(&_configs_lock);
	FOREACH(node, &_configs)
	{
		if (index == 0)
		{
			conf = (usb_device_configuration_t *)node;
			break;
		}
		index--;
	}
	exos_mutex_unlock(&_configs_lock);
	return conf;
}

unsigned usb_device_config_get_count()
{
	exos_mutex_lock(&_configs_lock);
	unsigned count = list_get_count(&_configs);
	exos_mutex_unlock(&_configs_lock);
	return count;
}

void usb_device_interface_create(usb_device_interface_t *iface, const usb_device_interface_driver_t *driver)
{
	*iface = (usb_device_interface_t) { .Index = -1, .Driver = driver };
}

void usb_device_config_add_interface(usb_device_configuration_t *conf, usb_device_interface_t *iface, const void *instance_data)
{
	exos_mutex_lock(&_configs_lock);

	if (conf == nullptr || iface == nullptr || iface->Driver == nullptr)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
	if (list_find_node(&conf->Interfaces, (node_t *)iface))
		kernel_panic(KERNEL_ERROR_LIST_ALREADY_CONTAINS_NODE);

	iface->Index = list_get_count(&conf->Interfaces);
	list_add_tail(&conf->Interfaces, (node_t *)iface);

	const usb_device_interface_driver_t *driver = iface->Driver;
	if (driver->Initialize != nullptr)
	{
		if (!driver->Initialize(iface, instance_data))
			kernel_panic(KERNEL_ERROR_NOT_ENOUGH_MEMORY);
	}

	exos_mutex_unlock(&_configs_lock);
}

static unsigned _fill_config(usb_configuration_descriptor_t *desc, unsigned char index, unsigned *plength)
{
	unsigned conf_length = *plength;
	usb_device_configuration_t *conf = usb_device_config_get(index);
	if (conf == nullptr)
	{
		verbose(VERBOSE_DEBUG, "usb_conf", "fill_conf_descriptor(%d) FAILED", (unsigned)index);
		return  0;
	}
	const usb_device_configuration_driver_t *driver = conf->Driver;
	if (driver == nullptr || driver->FillConfigurationDescriptor == nullptr)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);

	desc->ConfigurationValue = conf->Value;
	desc->ConfigurationIndex = index;
	unsigned total = 0;
	unsigned done = driver->FillConfigurationDescriptor(index, desc, USB_CONF_BUFFER_SIZE - total);
	// NOTE: driver must fill Attributes/MaxPower fields
	ASSERT(done >= sizeof(usb_configuration_descriptor_t), KERNEL_ERROR_KERNEL_PANIC);
	if (desc->MaxPower != 0)
		desc->Attributes |= USB_CONFIG_BUS_POWERED;	// USB2.0 spec mandates bit set even when self-powered 

	total += done;
	ASSERT(total <= USB_CONF_BUFFER_SIZE, KERNEL_ERROR_KERNEL_PANIC);

	unsigned if_index = 0;
	FOREACH(node, &conf->Interfaces)
	{
		usb_device_interface_t *iface = (usb_device_interface_t *)node;
		const usb_device_interface_driver_t *driver = iface->Driver;
		ASSERT(driver != NULL, KERNEL_ERROR_NULL_POINTER);
		ASSERT(driver->FillInterfaceDescriptor != NULL && driver->FillEndpointDescriptor != NULL, KERNEL_ERROR_NULL_POINTER);

		for(unsigned alt_setting = 0; alt_setting <= iface->AlternateSettings; alt_setting++)
		{
			void *ptr = (void *)desc + total;
			usb_interface_descriptor_t *if_desc = (usb_interface_descriptor_t *)ptr;
			*if_desc = (usb_interface_descriptor_t) { .InterfaceNumber = iface->Index, .AlternateSetting = alt_setting };
			unsigned size = driver->FillInterfaceDescriptor(iface, if_desc, USB_CONF_BUFFER_SIZE - total);
			ASSERT(size != 0 && size >= sizeof(usb_interface_descriptor_t), KERNEL_ERROR_KERNEL_PANIC);

			if_desc->Header = (usb_descriptor_header_t) { .Length = size, .DescriptorType = USB_DESCRIPTOR_TYPE_INTERFACE };
			total += size;
			ASSERT(total <= USB_CONF_BUFFER_SIZE, KERNEL_ERROR_KERNEL_PANIC);
			ptr += size;

			if (driver->FillClassDescriptor != nullptr)
			{
				usb_descriptor_header_t *class_desc_hdr = (usb_descriptor_header_t *)ptr;
				size = driver->FillClassDescriptor(iface, ptr, USB_CONF_BUFFER_SIZE - total);
				// NOTE: class driver must fill header fields
				if (size != 0)
				{
					ASSERT(class_desc_hdr->Length == size, KERNEL_ERROR_KERNEL_PANIC);
					total += size;
					ptr += size;
				}
			}

			for(unsigned ep_index = 0; ep_index < if_desc->NumEndpoints; ep_index++) 
			{
				ASSERT(total < conf_length, KERNEL_ERROR_KERNEL_PANIC);
				size = driver->FillEndpointDescriptor(iface, ep_index, (usb_endpoint_descriptor_t *)ptr, USB_CONF_BUFFER_SIZE - total);
				if (size == 0) break;
				*(usb_descriptor_header_t *)ptr = (usb_descriptor_header_t) { .Length = size, .DescriptorType = USB_DESCRIPTOR_TYPE_ENDPOINT };
				total += size;
				ptr += size;
			}
		}
		if_index++;
	}

	desc->TotalLength = HTOUSB16(total);
	desc->NumInterfaces = if_index;
	*plength = total;
	return sizeof(usb_configuration_descriptor_t);
}

static int _fill_string(usb_string_descriptor_t *desc, unsigned char index)
{
	int length = 0;
	if (index == 0)
	{
		desc->String[length++] = (usb16_t)HTOUSB16(0x0409);	// English - United States
	}
	else
	{
		const char *str = usb_device_config_get_string(index);
		if (str != nullptr)
		{
			for(int i = 0; i < 255; i++)
			{
				char c = str[i];
				if (c == '\0') break;
				desc->String[length++] = HTOUSB16(c);
			}
		}
	}
	return sizeof(usb_descriptor_header_t) + (length << 1);
	return length;
}



usb_descriptor_header_t *usb_device_config_get_descriptor(unsigned short req_value, unsigned short req_index, unsigned *plength)
{
	usb_descriptor_type_t desc_type = req_value >> 8;
	int desc_index = req_value & 0xFF;
    usb_descriptor_header_t *desc = (usb_descriptor_header_t *)_desc_buffer;
	int length = 0;

	// NOTE: Standard descriptors 
	switch(desc_type)
	{
		case USB_DESCRIPTOR_TYPE_DEVICE:
			length = usb_device_fill_device_descriptor((usb_device_descriptor_t *)desc);
			*plength = length;
			break;
		case USB_DESCRIPTOR_TYPE_CONFIGURATION:
			length = _fill_config((usb_configuration_descriptor_t *)desc, desc_index, plength);
			//NOTE: returns data beyond the end of first descriptor (configuration data)
			break;
		case USB_DESCRIPTOR_TYPE_STRING:
			length = _fill_string((usb_string_descriptor_t *)desc, desc_index);
			*plength = length;
			break;
		case USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER:
			//NOTE: this descriptor is rejected for full-speed and low-speed devices
			break;
		default:
			switch(desc_type & 0x3f)
			{
				case USB_REQTYPE_CLASS:
					length = usb_device_fill_class_descriptor(desc, req_value, req_index, *plength);
					*plength = length;
					break;
				case USB_REQTYPE_VENDOR:
					length = usb_device_fill_vendor_descriptor(desc, req_value, req_index, *plength);
					*plength = length;
					break;
			}
			break;
	}
	
	if (length != 0)
	{
		// TODO: check length
		*desc = (usb_descriptor_header_t) { .Length = length, .DescriptorType = desc_type };
	}
	else desc = nullptr;
	return desc;
}


unsigned usb_device_config_add_string(usb_device_string_t *str, const char *value)
{
	exos_mutex_lock(&_strings_lock);
	unsigned count = list_get_count(&_strings) + 1;
	*str = (usb_device_string_t) { .Index = count, .String = value };
	list_add_tail(&_strings, &str->Node);
	exos_mutex_unlock(&_strings_lock);
	return count;
}

const char *usb_device_config_get_string(int index)
{
	const char *str = nullptr;
	exos_mutex_lock(&_strings_lock);
	FOREACH(node, &_strings)
	{
		usb_device_string_t *str_node = (usb_device_string_t *)node;
		if (str_node->Index == index)
		{
			str = str_node->String;
			break;
		}
	}
	exos_mutex_unlock(&_strings_lock);
	return str;
}

#ifndef USB_USER_DEVICE_CLASS
#define USB_USER_DEVICE_CLASS 0 // by interface
#define USB_USER_DEVICE_SUBCLASS 0 // by interface
#define USB_USER_DEVICE_PROTOCOL 0
#endif

#ifndef USB_USER_DEVICE_VERSION
#define USB_USER_DEVICE_VERSION	0
#endif

__weak
int usb_device_fill_device_descriptor(usb_device_descriptor_t *desc)
{
	desc->USBVersion = HTOUSB16(USB_USB_2_0);
	desc->DeviceClass = USB_USER_DEVICE_CLASS;
	desc->DeviceSubClass = USB_USER_DEVICE_SUBCLASS;
	desc->DeviceProtocol = USB_USER_DEVICE_PROTOCOL;
	desc->MaxPacketSize = USB_MAX_PACKET0;
#ifdef USB_USER_DEVICE_VENDORID
	desc->VendorId = HTOUSB16(USB_USER_DEVICE_VENDORID);
	desc->ProductId = HTOUSB16(USB_USER_DEVICE_PRODUCTID);
#else
#warning "Undefined usb device IDs"
#endif
	desc->DeviceVersion = HTOUSB16(USB_USER_DEVICE_VERSION);
	desc->ManufacturerIndex = _manufacturer_str;
	desc->ProductIndex = _product_str;
	desc->SerialNumberIndex = _serialnumber_str;
	desc->NumConfigurations = usb_device_config_get_count();
	return sizeof(usb_device_descriptor_t);
}

__weak
bool usb_device_setup_class_request(usb_request_t *req, void **pdata, int *plength)
{
#ifdef DEBUG
	kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
#endif
	return false;
}

__weak
bool usb_device_setup_vendor_request(usb_request_t *req, void **pdata, int *plength)
{
#ifdef DEBUG
	kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
#endif
	return false;
}

__weak
int usb_device_fill_class_descriptor(usb_descriptor_header_t *desc, unsigned short req_value, unsigned short req_index, int max_plength)
{
#ifdef DEBUG
	kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
#endif
	return 0;
}

__weak
int usb_device_fill_vendor_descriptor(usb_descriptor_header_t *desc, unsigned short req_value, unsigned short req_index, int max_plength)
{
#ifdef DEBUG
	kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
#endif
	return  0;
}


