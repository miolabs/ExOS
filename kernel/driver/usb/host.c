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
	device->State = USB_HOST_DEVICE_CREATED;

	// setup control pipe with initial 8 byte max transfer size
	USB_HOST_PIPE *pipe = &device->ControlPipe;
	*pipe = (USB_HOST_PIPE) { .Device = device, .EndpointType = USB_TT_CONTROL, .MaxPacketSize = 8 }; 
}

void usb_host_destroy_device(USB_HOST_DEVICE *device)
{
#ifdef DEBUG
	if (device == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	FOREACH(node, &device->Functions)
	{
		USB_HOST_FUNCTION *func = (USB_HOST_FUNCTION *)node;
		usb_host_destroy_function(func);
	}
}

void usb_host_create_function(USB_HOST_FUNCTION *func, USB_HOST_DEVICE *device, const USB_HOST_FUNCTION_DRIVER *driver)
{
	*func = (USB_HOST_FUNCTION) {
#ifdef DEBUG
		.Node = (EXOS_NODE) { .Type = EXOS_NODE_UNKNOWN },
#endif
		.Device = device, .Driver = driver };
}

void usb_host_destroy_function(USB_HOST_FUNCTION *func)
{
	const USB_HOST_FUNCTION_DRIVER *driver = func->Driver;
    driver->Stop(func);
}

void usb_host_init_pipe_from_descriptor(USB_HOST_DEVICE *device, USB_HOST_PIPE *pipe, USB_ENDPOINT_DESCRIPTOR *ep_desc)
{
	*pipe = (USB_HOST_PIPE) {
		.Device = device,
		.EndpointType = ep_desc->AttributesBits.TransferType,
		.Direction = ep_desc->AddressBits.Input ? USB_DEVICE_TO_HOST : USB_HOST_TO_DEVICE,
		.MaxPacketSize = USB16TOH(ep_desc->MaxPacketSize),
		.EndpointNumber = ep_desc->AddressBits.EndpointNumber,
		.InterruptInterval = ep_desc->Interval };
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

int usb_host_stop_pipe(USB_HOST_PIPE *pipe)
{
	USB_HOST_DEVICE *device = pipe->Device;
	exos_mutex_lock(&device->ControlMutex);
	const USB_HOST_CONTROLLER_DRIVER *hcd = device->Controller;
	int done = hcd->StopPipe(pipe);
	exos_mutex_unlock(&device->ControlMutex);
	return done;	
}

int usb_host_bulk_transfer(USB_HOST_PIPE *pipe, void *data, int length, unsigned long timeout)
{
	USB_HOST_DEVICE *device = pipe->Device;
	const USB_HOST_CONTROLLER_DRIVER *hcd = device->Controller;

	int done = 0;
	USB_REQUEST_BUFFER urb;
	usb_host_urb_create(&urb, pipe);
	if (hcd->BeginBulkTransfer(&urb, data, length))
	{
		done = hcd->EndBulkTransfer(&urb, timeout);
	}
	return done;
}

int usb_host_begin_bulk_transfer(USB_REQUEST_BUFFER *urb, void *data, int length)
{
	if (urb == NULL || urb->Pipe == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
	
	USB_HOST_DEVICE *device = urb->Pipe->Device;
	const USB_HOST_CONTROLLER_DRIVER *hcd = device->Controller;
	int done = hcd->BeginBulkTransfer(urb, data, length);
	return done;
}

int usb_host_end_bulk_transfer(USB_REQUEST_BUFFER *urb, unsigned long timeout)
{
	if (urb == NULL || urb->Pipe == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
	
	USB_HOST_DEVICE *device = urb->Pipe->Device;
	const USB_HOST_CONTROLLER_DRIVER *hcd = device->Controller;
	int done = hcd->EndBulkTransfer(urb, timeout);
	return done;
}

int usb_host_ctrl_setup(USB_HOST_DEVICE *device, const USB_REQUEST *request, void *data, int length)
{
	if (device == NULL || device->State != USB_HOST_DEVICE_ATTACHED) 
		return 0;

	exos_mutex_lock(&device->ControlMutex);
	const USB_HOST_CONTROLLER_DRIVER *hcd = device->Controller;
	device->ControlBuffer = *request;
	int done = (length != 0 &&
		(request->RequestType & USB_REQTYPE_DIRECTION_MASK) == USB_REQTYPE_DEVICE_TO_HOST) ?
		hcd->CtrlSetupRead(device, &device->ControlBuffer, sizeof(USB_REQUEST), data, length) :
		hcd->CtrlSetupWrite(device, &device->ControlBuffer, sizeof(USB_REQUEST), data, length);
	exos_mutex_unlock(&device->ControlMutex);
    return done;
}

int usb_host_read_device_descriptor(USB_HOST_DEVICE *device, int desc_type, int desc_index, void *data, int length)
{
	USB_REQUEST req = (USB_REQUEST) {
		.RequestType = USB_REQTYPE_DEVICE_TO_HOST | USB_REQTYPE_RECIPIENT_DEVICE,
		.RequestCode = USB_REQUEST_GET_DESCRIPTOR,
		.Value = (desc_type << 8) | desc_index, .Index = 0, .Length = length };
	return usb_host_ctrl_setup(device, &req, data, length);
}

int usb_host_read_string_descriptor(USB_HOST_DEVICE *device, int lang_id, int str_index, void *data, int length)
{
	USB_REQUEST req = (USB_REQUEST) {
		.RequestType = USB_REQTYPE_DEVICE_TO_HOST | USB_REQTYPE_RECIPIENT_DEVICE,
		.RequestCode = USB_REQUEST_GET_DESCRIPTOR,
		.Value = (USB_DESCRIPTOR_TYPE_STRING << 8) | str_index, .Index = lang_id, .Length = length };
	return usb_host_ctrl_setup(device, &req, data, length);
}

int usb_host_read_if_descriptor(USB_HOST_DEVICE *device, int interface, int desc_type, int desc_index, void *data, int length)
{
	USB_REQUEST req = (USB_REQUEST) {
		.RequestType = USB_REQTYPE_DEVICE_TO_HOST | USB_REQTYPE_RECIPIENT_INTERFACE,
		.RequestCode = USB_REQUEST_GET_DESCRIPTOR,
		.Value = (desc_type << 8) | desc_index, .Index = interface, .Length = length };
	return usb_host_ctrl_setup(device, &req, data, length);
}

int usb_host_set_address(USB_HOST_DEVICE *device, int addr)
{
	USB_REQUEST req = (USB_REQUEST) {
		.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_RECIPIENT_DEVICE,
		.RequestCode = USB_REQUEST_SET_ADDRESS,
		.Value = addr, .Index = 0, .Length = 0 };
	return usb_host_ctrl_setup(device, &req, NULL, 0);
}

int usb_host_set_interface(USB_HOST_DEVICE *device, int interface, int alternate_setting)
{
	USB_REQUEST req = (USB_REQUEST) {
		.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_RECIPIENT_INTERFACE,
		.RequestCode = USB_REQUEST_SET_INTERFACE,
		.Value = alternate_setting, .Index = interface, .Length = 0 };
	return usb_host_ctrl_setup(device, &req, NULL, 0);
}

void usb_host_urb_create(USB_REQUEST_BUFFER *urb, USB_HOST_PIPE *pipe)
{
	urb->Pipe = pipe;
	exos_event_create(&urb->Event);
	urb->Status = URB_STATUS_EMPTY;
}


int usb_host_create_child_device(USB_HOST_DEVICE *hub, USB_HOST_DEVICE *child, int port, USB_HOST_DEVICE_SPEED speed)
{
#ifdef DEBUG
	if (hub == NULL || child == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	const USB_HOST_CONTROLLER_DRIVER *hcd = hub->Controller;
#ifdef DEBUG
	if (hcd == NULL || hcd->CreateDevice == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	int done = hcd->CreateDevice(child, port, speed);
	return done;
}

void usb_host_destroy_child_device(USB_HOST_DEVICE *child)
{
#ifdef DEBUG
	if (child == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	const USB_HOST_CONTROLLER_DRIVER *hcd = child->Controller;
#ifdef DEBUG
	if (hcd == NULL || hcd->DestroyDevice == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	hcd->DestroyDevice(child);
}





