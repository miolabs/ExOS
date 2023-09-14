#include <usb/configuration.h>
#include <support/apple/iap2/iap2_device.h>
#include <support/usb/device/cdc_ncm.h>
#include <kernel/verbose.h>
#include <kernel/panic.h>
#include "board.h"

static unsigned _fill_conf_desc(unsigned conf_index, usb_configuration_descriptor_t *conf_desc, unsigned buffer_size);

static usb_device_configuration_t _conf;
static const usb_device_configuration_driver_t _conf_driver = { 
	.FillConfigurationDescriptor = _fill_conf_desc };
static usb_device_interface_t _if1;
static usb_device_interface_t _if2;

void usb_device_add_configurations()
{
	usb_device_config_create(&_conf, 0x11, &_conf_driver);

#if !defined USE_OLD_IAP && !defined DISABLE_IAP2_DEVICE 
	usb_device_interface_create(&_if1, __usb_iap2_device_driver);
	usb_device_config_add_interface(&_conf, &_if1, nullptr);
#endif

	ASSERT(__board_ethernet_adapter != NULL, KERNEL_ERROR_NULL_POINTER);
	usb_device_interface_create(&_if2, __usb_cdc_ncm_device_driver);
	usb_device_config_add_interface(&_conf, &_if2, __board_ethernet_adapter);

	usb_device_config_register(&_conf);
}

static unsigned _fill_conf_desc(unsigned conf_index, usb_configuration_descriptor_t *conf_desc, unsigned buffer_size)
{
	if (buffer_size < sizeof(usb_configuration_descriptor_t))
		return 0;

	switch(conf_index)
	{
		case 0:
			conf_desc->Attributes = USB_CONFIG_SELF_POWERED;
			conf_desc->MaxPower = USB_CONFIG_POWER(50);	// bus-power, mA
			return sizeof(usb_configuration_descriptor_t);
	}
	return 0;
}

bool usb_device_setup_vendor_request(usb_request_t *req, void **pdata, int *plength)
{
	verbose(VERBOSE_DEBUG, "usb_config", "vendor setup request (unhandled)");
	return false;	// ftdi_vendor_request(req, pdata, plength);
}
