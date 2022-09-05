#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include <usb/usb.h>
#include <kernel/event.h>
#include <kernel/dispatch.h>
#include <stdbool.h>

#ifndef USB_CONTROL_MAX_DATA
#define USB_CONTROL_MAX_DATA 512
#endif

#ifndef USB_DEVICE_ENDPOINTS
#define USB_DEVICE_ENDPOINTS 16
#endif

#ifndef USB_MAX_PACKET0
#define USB_MAX_PACKET0 8
#endif

typedef enum 
{
	USB_IOF_NONE = 0,
	USB_IOF_DMA = (1<<0),
	USB_IOF_AUTO_STALL = (1<<1),
	USB_IOF_SHORT_PACKET_END = (1<<2),
} usb_io_flags_t;

typedef enum
{
	USB_IOSTA_UNKNOWN = 0,
	USB_IOSTA_OUT_WAIT,
	USB_IOSTA_IN_WAIT,
	USB_IOSTA_IN_COMPLETE,
	USB_IOSTA_DONE,
	USB_IOSTA_ERROR,
} usb_io_status_t;

typedef struct
{
	usb_io_status_t Status;
	usb_io_flags_t Flags;
	void *Data;
	unsigned short Length;
	unsigned short Done;
	event_t *Event;
} usb_io_buffer_t;

typedef enum
{
	USB_IFSTA_STOPPED = 0,
	USB_IFSTA_STARTING,
	USB_IFSTA_STARTED,
	USB_IFSTA_STOPPING,
} usb_if_status_t;

typedef struct
{
	node_t Node;
	unsigned char Index;
	const char *String;
} usb_device_string_t;

typedef struct usb_device_interface_driver usb_device_interface_driver_t;

typedef struct
{
	node_t Node;
	const usb_device_interface_driver_t *Driver;
	void *DriverContext;
	usb_device_string_t Name;
	usb_if_status_t Status;
	unsigned char Index;
} usb_device_interface_t;

struct usb_device_interface_driver
{
	bool (*Initialize)(usb_device_interface_t *iface, const void *instance_data);
	unsigned (*MeasureInterfaceDescriptors)(usb_device_interface_t *iface);
	unsigned (*FillInterfaceDescriptor)(usb_device_interface_t *iface, usb_interface_descriptor_t *if_desc, unsigned buffer_size);
	unsigned (*FillClassDescriptor)(usb_device_interface_t *iface, usb_descriptor_header_t *class_desc, unsigned buffer_size);
	unsigned (*FillEndpointDescriptor)(usb_device_interface_t *iface, unsigned ep_index, usb_endpoint_descriptor_t *ep_desc, unsigned buffer_size);
	bool (*Start)(usb_device_interface_t *iface, unsigned char alternate_setting, dispatcher_context_t *context);
	void (*Stop)(usb_device_interface_t *iface);

	bool (*VendorRequest)(usb_device_interface_t *iface, usb_request_t *req, void **pdata, unsigned *plength);
	bool (*InterfaceRequest)(usb_device_interface_t *iface, usb_request_t *req, void **pdata, unsigned *plength);
	bool (*ClassRequest)(usb_device_interface_t *iface, usb_request_t *req, void **pdata, unsigned *plength);
};

// prototypes
bool usb_device_initialize();
void usb_set_rx_buffer(unsigned ep_num, usb_io_buffer_t *rx_buffer);
void usb_set_tx_buffer(unsigned ep_num, usb_io_buffer_t *tx_buffer);

#endif // USB_DEVICE_H
 