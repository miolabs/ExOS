#ifndef USB_HOST_H
#define USB_HOST_H

#include <kernel/list.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <usb/usb.h>

typedef const struct _USB_HOST_FUNCTION_DRIVER USB_HOST_FUNCTION_DRIVER;
typedef const struct _USB_HOST_CONTROLLER_DRIVER USB_HOST_CONTROLLER_DRIVER;
typedef struct _USB_HOST_DEVICE USB_HOST_DEVICE;
typedef struct _USB_REQUEST_BUFFER USB_REQUEST_BUFFER;

typedef struct _USB_HOST_PIPE
{
	USB_HOST_DEVICE *Device;
	void *Endpoint;
	USB_TRANSFERTYPE EndpointType;
    USB_DIRECTION Direction;
	unsigned char MaxPacketSize;
	unsigned char EndpointNumber;
	unsigned char InterruptInterval;
	unsigned char Reserved;
} USB_HOST_PIPE;

typedef enum
{
	USB_HOST_DEVICE_LOW_SPEED = 0,
	USB_HOST_DEVICE_FULL_SPEED = 1,
	USB_HOST_DEVICE_HIGH_SPEED = 2,
} USB_HOST_DEVICE_SPEED;

typedef enum
{
	USB_HOST_DEVICE_CREATED = 0,
	USB_HOST_DEVICE_ATTACHED,
	USB_HOST_DEVICE_DETACHED,
} USB_HOST_DEVICE_STATE;

struct _USB_HOST_DEVICE
{
	EXOS_NODE Node;
	const USB_HOST_CONTROLLER_DRIVER *Controller;
	EXOS_LIST Functions;
	USB_HOST_PIPE ControlPipe;
	EXOS_MUTEX ControlMutex;
	USB_REQUEST	ControlBuffer;

	unsigned char Port;
	unsigned char Speed;
	unsigned char Address;
	unsigned char Reserved;

	unsigned short Vendor;
	unsigned short Product;
    USB_HOST_DEVICE_STATE State;
};

typedef enum
{
	URB_STATUS_EMPTY = 0,
	URB_STATUS_ISSUED,
	URB_STATUS_FAILED,
	URB_STATUS_DONE,
	USB_STATUS_CANCELLED,
} URB_STATUS;

typedef struct
{
	EXOS_NODE Node;
	USB_HOST_DEVICE *Device;
	USB_HOST_FUNCTION_DRIVER *Driver;
} USB_HOST_FUNCTION;

struct _USB_HOST_FUNCTION_DRIVER
{
	USB_HOST_FUNCTION *(*CheckInterface)(USB_HOST_DEVICE *device, USB_CONFIGURATION_DESCRIPTOR *conf_desc, USB_DESCRIPTOR_HEADER *fn_desc);
	void (*Start)(USB_HOST_FUNCTION *func);
	void (*Stop)(USB_HOST_FUNCTION *func);
};

struct _USB_REQUEST_BUFFER
{
	EXOS_NODE Node;
	USB_HOST_PIPE *Pipe;
	EXOS_EVENT Event;
	void *Data;
	unsigned short Length;
	unsigned short Done;
	void *UserState;
	URB_STATUS Status;
};

struct _USB_HOST_CONTROLLER_DRIVER
{
	int (*CtrlSetupRead)(USB_HOST_DEVICE *device, void *setup_data, int setup_length, void *in_data, int in_length);
	int (*CtrlSetupWrite)(USB_HOST_DEVICE *device, void *setup_data, int setup_length, void *out_data, int out_length);
	int (*StartPipe)(USB_HOST_PIPE *pipe);
	int (*StopPipe)(USB_HOST_PIPE *pipe);
	int (*BeginBulkTransfer)(USB_REQUEST_BUFFER *urb, void *data, int length);
	int (*EndBulkTransfer)(USB_REQUEST_BUFFER *urb, unsigned long timeout);
};

typedef struct
{
	EXOS_NODE Node;
	const USB_HOST_FUNCTION_DRIVER *Driver;
} USB_HOST_FUNCTION_DRIVER_NODE;

typedef int (*USB_HOST_DRIVER_ENUMERATE_CALLBACK)(USB_HOST_FUNCTION_DRIVER *driver, void *arg);

// prototypes
int usb_host_initialize();
void usb_host_driver_register(USB_HOST_FUNCTION_DRIVER_NODE *driver_node);
int usb_host_driver_enumerate(USB_HOST_DRIVER_ENUMERATE_CALLBACK callback, void *arg);
void usb_host_create_device(USB_HOST_DEVICE *device, USB_HOST_CONTROLLER_DRIVER *hcd, int port, USB_HOST_DEVICE_SPEED speed);
void usb_host_destroy_device(USB_HOST_DEVICE *device);
void usb_host_create_function(USB_HOST_FUNCTION *func, USB_HOST_DEVICE *device, const USB_HOST_FUNCTION_DRIVER *driver);
void usb_host_destroy_function(USB_HOST_FUNCTION *func);
void usb_host_init_pipe_from_descriptor(USB_HOST_DEVICE *device, USB_HOST_PIPE *pipe, USB_ENDPOINT_DESCRIPTOR *ep_desc);
int usb_host_start_pipe(USB_HOST_PIPE *pipe);
int usb_host_stop_pipe(USB_HOST_PIPE *pipe);
void usb_host_urb_create(USB_REQUEST_BUFFER *urb, USB_HOST_PIPE *pipe);
int usb_host_bulk_transfer(USB_HOST_PIPE *pipe, void *data, int length, unsigned long timeout);
int usb_host_begin_bulk_transfer(USB_REQUEST_BUFFER *urb, void *data, int length);
int usb_host_end_bulk_transfer(USB_REQUEST_BUFFER *urb, unsigned long timeout);

int usb_host_ctrl_setup(USB_HOST_DEVICE *device, const USB_REQUEST *request, void *data, int length);
int usb_host_read_device_descriptor(USB_HOST_DEVICE *device, int desc_type, int desc_index, void *data, int length);
int usb_host_read_string_descriptor(USB_HOST_DEVICE *device, int lang_id, int str_index, void *data, int length);
int usb_host_read_if_descriptor(USB_HOST_DEVICE *device, int interface, int desc_type, int desc_index, void *data, int length);
int usb_host_set_address(USB_HOST_DEVICE *device, int addr);
int usb_host_set_interface(USB_HOST_DEVICE *device, int interface, int alternate_setting);

void usb_host_add_drivers() __weak;

#endif // USB_HOST_H
