#include "host.h"
#include <support/usb/host_hal.h>
#include <kernel/mutex.h>
#include <kernel/panic.h>

static EXOS_LIST _drivers;
static EXOS_MUTEX _drivers_lock;

int usb_host_initialize()
{	
	list_initialize(&_drivers);
	exos_mutex_create(&_drivers_lock);
	usb_host_add_drivers();

	hal_usb_host_initialize();
	return 1;
}

void usb_host_driver_register(USB_HOST_FUNCTION_DRIVER_NODE *driver_node)
{
	exos_mutex_lock(&_drivers_lock);

	if (driver_node == NULL || driver_node->Driver == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
	if (list_find_node(&_drivers, (EXOS_NODE *)driver_node))
		kernel_panic(KERNEL_ERROR_LIST_ALREADY_CONTAINS_NODE);

	list_add_tail(&_drivers, (EXOS_NODE *)driver_node);
	exos_mutex_unlock(&_drivers_lock);
}

int usb_host_driver_enumerate(USB_HOST_DRIVER_ENUMERATE_CALLBACK callback, void *arg)
{
	exos_mutex_lock(&_drivers_lock);
	int done = 0;
	FOREACH(node, &_drivers)
	{
		USB_HOST_FUNCTION_DRIVER_NODE *driver_node = (USB_HOST_FUNCTION_DRIVER_NODE *)node;
		done = callback(driver_node->Driver, arg);
		if (done) break;
	}
	exos_mutex_unlock(&_drivers_lock);
	return done;
}

void usb_host_create_device(USB_HOST_DEVICE *device, USB_HOST_CONTROLLER_DRIVER *hcd, int port, USB_HOST_DEVICE_SPEED speed)
{
	device->Controller = hcd;
	list_initialize(&device->Functions);
	exos_mutex_create(&device->ControlMutex);

	device->Port = port;
	device->Speed = speed;
	device->Address = 0;

	device->Vendor = device->Product = 0;

	// setup control pipe with initial 8 byte max transfer size
	USB_HOST_PIPE *pipe = &device->ControlPipe;
	*pipe = (USB_HOST_PIPE) { .Device = device, .EndpointType = USB_TT_CONTROL, .MaxPacketSize = 8 }; 
	exos_event_create(&pipe->Event);
}

void usb_host_create_function(USB_HOST_FUNCTION *func, USB_HOST_DEVICE *device, USB_HOST_FUNCTION_DRIVER *driver)
{
	*func = (USB_HOST_FUNCTION) {
#ifdef DEBUG
		.Node = (EXOS_NODE) { .Type = EXOS_NODE_UNKNOWN },
#endif
		.Device = device, .Driver = driver };
}

void usb_host_init_pipe_from_descriptor(USB_HOST_DEVICE *device, USB_HOST_PIPE *pipe, USB_ENDPOINT_DESCRIPTOR *ep_desc)
{
	*pipe= (USB_HOST_PIPE) {
		.Device = device,
		.EndpointType = ep_desc->AttributesBits.TransferType,
		.Direction = ep_desc->AddressBits.Input ? USB_DEVICE_TO_HOST : USB_HOST_TO_DEVICE,
		.MaxPacketSize = USB16TOH(ep_desc->MaxPacketSize),
		.EndpointNumber = ep_desc->AddressBits.EndpointNumber,
		.InterruptInterval = ep_desc->Interval };
	exos_event_create(&pipe->Event);
}

int usb_host_start_pipe(USB_HOST_PIPE *pipe)
{
	USB_HOST_DEVICE *device = pipe->Device;
	exos_mutex_lock(&device->ControlMutex);
	const USB_HOST_CONTROLLER_DRIVER *hcd = device->Controller;
	int done = hcd->StartPipe(pipe);
	exos_mutex_unlock(&device->ControlMutex);
	return done;
}

int usb_host_bulk_transfer(USB_HOST_PIPE *pipe, void *data, int length)
{
	USB_HOST_DEVICE *device = pipe->Device;
	exos_mutex_lock(&device->ControlMutex);
	const USB_HOST_CONTROLLER_DRIVER *hcd = device->Controller;
	int done = hcd->BulkTransfer(pipe, data, length);
	exos_mutex_unlock(&device->ControlMutex);
	return done;
}

int usb_host_ctrl_setup_read(USB_HOST_DEVICE *device, void *setup_data, int setup_length, void *data, int length)
{
	exos_mutex_lock(&device->ControlMutex);
	const USB_HOST_CONTROLLER_DRIVER *hcd = device->Controller;
	int done = hcd->CtrlSetupRead(device, setup_data, setup_length, data, length);
	exos_mutex_unlock(&device->ControlMutex);
    return done;
}

int usb_host_ctrl_setup_write(USB_HOST_DEVICE *device, void *setup_data, int setup_length, void *data, int length)
{
	exos_mutex_lock(&device->ControlMutex);
	const USB_HOST_CONTROLLER_DRIVER *hcd = device->Controller;
	int done = hcd->CtrlSetupWrite(device, setup_data, setup_length, data, length);
	exos_mutex_unlock(&device->ControlMutex);
    return done;
}

int usb_host_ctrl_setup(USB_HOST_DEVICE *device, void *setup_data, int setup_length)
{
	exos_mutex_lock(&device->ControlMutex);
	const USB_HOST_CONTROLLER_DRIVER *hcd = device->Controller;
	int done = hcd->CtrlSetup(device, setup_data, setup_length);
	exos_mutex_unlock(&device->ControlMutex);
    return done;
}

int usb_host_read_descriptor(USB_HOST_DEVICE *device, int desc_type, int desc_index, void *data, int length)
{
	USB_REQUEST setup = (USB_REQUEST) {
		.RequestType = USB_REQTYPE_DEVICE_TO_HOST | USB_REQTYPE_RECIPIENT_DEVICE,
		.RequestCode = USB_REQUEST_GET_DESCRIPTOR,
		.Value = (desc_type << 8) | desc_index, .Index = 0, .Length = length };
	return usb_host_ctrl_setup_read(device, &setup, sizeof(USB_REQUEST), data, length);
}

int usb_host_set_address(USB_HOST_DEVICE *device, int addr)
{
	USB_REQUEST setup = (USB_REQUEST) {
		.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_RECIPIENT_DEVICE,
		.RequestCode = USB_REQUEST_SET_ADDRESS,
		.Value = addr, .Index = 0, .Length = 0 };
	return usb_host_ctrl_setup(device, &setup, sizeof(USB_REQUEST));
}

int usb_host_set_interface(USB_HOST_DEVICE *device, int interface, int alternate_setting)
{
	USB_REQUEST setup = (USB_REQUEST) {
		.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_RECIPIENT_INTERFACE,
		.RequestCode = USB_REQUEST_SET_INTERFACE,
		.Value = alternate_setting, .Index = interface, .Length = 0 };
	return usb_host_ctrl_setup(device, &setup, sizeof(USB_REQUEST));
}







