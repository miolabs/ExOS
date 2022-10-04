#ifndef USB_HOST_H
#define USB_HOST_H

#include <stdbool.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <usb/usb.h>

typedef const struct __usb_host_function_driver usb_host_function_driver_t;
typedef const struct __usb_host_controller_driver usb_host_controller_driver_t;

typedef struct __usb_host_controller usb_host_controller_t; 
typedef struct __usb_host_device usb_host_device_t;
typedef struct __usb_request_buffer usb_request_buffer_t;

typedef struct
{
	usb_host_device_t *Device;
	void *Endpoint;
	usb_transfer_type_t EndpointType;
    usb_direction_t Direction;
	unsigned char MaxPacketSize;
	unsigned char EndpointNumber;
	unsigned char InterruptInterval;
} usb_host_pipe_t;

typedef enum
{
	USB_HOST_DEVICE_LOW_SPEED = 0,
	USB_HOST_DEVICE_FULL_SPEED = 1,
	USB_HOST_DEVICE_HIGH_SPEED = 2,
} usb_host_device_speed_t;

typedef enum
{
	USB_HOST_DEVICE_CREATED = 0,
	USB_HOST_DEVICE_ATTACHED,
	USB_HOST_DEVICE_DETACHED,
} usb_host_device_state_t;

struct __usb_host_device
{
	node_t Node;
	usb_host_controller_t *Controller;
	list_t Functions;
	usb_host_pipe_t ControlPipe;
	mutex_t ControlMutex;
	usb_request_t ControlBuffer;

	unsigned char Port;
	unsigned char Speed;
	unsigned char Address;
	unsigned char Reserved;

	unsigned short Vendor;
	unsigned short Product;
    usb_host_device_state_t State;
};

typedef enum
{
	URB_STATUS_EMPTY = 0,
	URB_STATUS_ISSUED,
	URB_STATUS_FAILED,
	URB_STATUS_DONE,
	USB_STATUS_CANCELLED,
} urb_status_t;

typedef struct
{
	node_t Node;
	usb_host_device_t *Device;
	usb_host_function_driver_t *Driver;
} usb_host_function_t;

struct __usb_host_function_driver
{
	usb_host_function_t *(*CheckInterface)(usb_host_device_t *device, usb_configuration_descriptor_t *conf_desc, usb_descriptor_header_t *fn_desc);
	void (*Start)(usb_host_function_t *func, usb_configuration_descriptor_t *conf_desc, usb_descriptor_header_t *fn_desc);
	void (*Stop)(usb_host_function_t *func);
};

typedef struct
{
	node_t Node;
	const usb_host_function_driver_t *Driver;
} usb_host_function_driver_node_t;

struct __usb_request_buffer
{
	node_t Node;
	usb_host_pipe_t *Pipe;
	event_t Event;
	void *Data;
	unsigned short Length;
	unsigned short Done;
	void *UserState;
	urb_status_t Status;
};

struct __usb_host_controller
{
	node_t Node;
	const usb_host_controller_driver_t *Driver;
	void *DriverState;
	event_t SOF;
	event_t RootHubEvent;
	unsigned char RootHubPorts;
	usb_host_device_t *Devices;
};

struct __usb_host_controller_driver
{
	bool (*CtrlSetupRead)(usb_host_device_t *device, void *setup_data, unsigned setup_length, void *in_data, unsigned in_length);
	bool (*CtrlSetupWrite)(usb_host_device_t *device, void *setup_data, unsigned setup_length, void *out_data, unsigned out_length);
	bool (*StartPipe)(usb_host_controller_t *hc, usb_host_pipe_t *pipe);
	bool (*StopPipe)(usb_host_controller_t *hc, usb_host_pipe_t *pipe);
	bool (*BeginTransfer)(usb_host_controller_t *hc, usb_request_buffer_t *urb, void *data, unsigned length);
	int (*EndTransfer)(usb_host_controller_t *hc, usb_request_buffer_t *urb, unsigned timeout);
	bool (*CreateDevice)(usb_host_controller_t *hc, usb_host_device_t *device, unsigned port, usb_host_device_speed_t speed);
	void (*DestroyDevice)(usb_host_controller_t *hc, usb_host_device_t *device);
	bool (*RequestRoleSwitch)(usb_host_controller_t *hc);
};

typedef bool (*usb_host_driver_enumerate_callback_t)(usb_host_function_driver_t *driver, void *arg);

// prototypes
void usb_host_controller_create(usb_host_controller_t *hc, const usb_host_controller_driver_t *driver, usb_host_device_t *devices, unsigned port_count);
bool usb_host_request_role_switch(usb_host_controller_t *hc);
void usb_host_wait_sof(usb_host_controller_t *hc);
void usb_host_driver_register(usb_host_function_driver_node_t *driver_node);
bool usb_host_driver_enumerate(usb_host_driver_enumerate_callback_t callback, void *arg);
usb_host_device_t *usb_host_create_root_device(usb_host_controller_t *hc, unsigned port, usb_host_device_speed_t speed);
bool usb_host_create_child_device(usb_host_device_t *hub, usb_host_device_t *child, unsigned port, usb_host_device_speed_t speed);
void usb_host_destroy_device(usb_host_device_t *child);
void usb_host_create_function(usb_host_function_t *func, usb_host_device_t *device, const usb_host_function_driver_t *driver);
void usb_host_destroy_function(usb_host_function_t *func);
void usb_host_init_pipe_from_descriptor(usb_host_device_t *device, usb_host_pipe_t *pipe, usb_endpoint_descriptor_t *ep_desc);
bool usb_host_start_pipe(usb_host_pipe_t *pipe);
bool usb_host_stop_pipe(usb_host_pipe_t *pipe);
void usb_host_urb_create(usb_request_buffer_t *urb, usb_host_pipe_t *pipe);
int usb_host_do_transfer(usb_host_pipe_t *pipe, void *data, unsigned length, unsigned timeout);
bool usb_host_begin_transfer(usb_request_buffer_t *urb, void *data, unsigned length);
int usb_host_end_transfer(usb_request_buffer_t *urb, unsigned timeout);

bool usb_host_ctrl_setup(usb_host_device_t *device, const usb_request_t *request, void *data, unsigned length);
bool usb_host_read_device_descriptor(usb_host_device_t *device, unsigned char desc_type, unsigned char desc_index, void *data, unsigned short length);
bool usb_host_read_string_descriptor(usb_host_device_t *device, unsigned short lang_id, unsigned char str_index, void *data, unsigned short length);
bool usb_host_read_if_descriptor(usb_host_device_t *device, unsigned short interface, unsigned char desc_type, unsigned char desc_index, void *data, unsigned short length);
bool usb_host_set_address(usb_host_device_t *device, unsigned char addr);
bool usb_host_set_interface(usb_host_device_t *device, unsigned short interface, unsigned short alternate_setting);

// overridables
void __usb_host_disabled(usb_host_controller_t *hc) __weak;

#endif // USB_HOST_H
