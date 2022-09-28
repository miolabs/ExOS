#include "host.h"
#include <kernel/mutex.h>
#include <support/services/init.h>
#include <kernel/panic.h>

static void _register();
EXOS_INITIALIZER(_init, EXOS_INIT_HW_DRIVER, _register);

static list_t _drivers;
static mutex_t _drivers_lock;

static void _register()
{
	list_initialize(&_drivers);
	exos_mutex_create(&_drivers_lock);
}

void usb_host_controller_create(usb_host_controller_t *hc, const usb_host_controller_driver_t *driver, usb_host_device_t *devices, unsigned port_count)
{
	ASSERT(hc != nullptr && driver != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(devices != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(port_count != 0, KERNEL_ERROR_KERNEL_PANIC);
	*hc = (usb_host_controller_t) {
		.Driver = driver,
		.RootHubPorts = port_count,
		.Devices = devices };
	exos_event_create(&hc->SOF, EXOS_EVENTF_AUTORESET);
	exos_event_create(&hc->RootHubEvent, EXOS_EVENTF_AUTORESET);
}

bool usb_host_controller_stop(usb_host_controller_t *hc)
{
	ASSERT(hc != nullptr, KERNEL_ERROR_NULL_POINTER);
	const usb_host_controller_driver_t *hcd = hc->Driver;
	ASSERT(hcd != nullptr, KERNEL_ERROR_NULL_POINTER);
	
	bool done = false;
	if (hcd->Stop != nullptr)
	{
		done = hcd->Stop(hc);
	}
	return done;
}

void usb_host_wait_sof(usb_host_controller_t *hc)
{
	ASSERT(hc != nullptr, KERNEL_ERROR_NULL_POINTER);
	exos_event_wait(&hc->SOF, EXOS_TIMEOUT_NEVER);
}

void usb_host_driver_register(usb_host_function_driver_node_t *driver_node)
{
	ASSERT(driver_node != nullptr, KERNEL_ERROR_NULL_POINTER);
	exos_mutex_lock(&_drivers_lock);

	ASSERT(driver_node != nullptr && driver_node->Driver != nullptr, KERNEL_ERROR_NULL_POINTER);
	if (list_find_node(&_drivers, &driver_node->Node))
		kernel_panic(KERNEL_ERROR_LIST_ALREADY_CONTAINS_NODE);

	list_add_tail(&_drivers, &driver_node->Node);
	exos_mutex_unlock(&_drivers_lock);
}

bool usb_host_driver_enumerate(usb_host_driver_enumerate_callback_t callback, void *arg)
{
	ASSERT(callback != nullptr, KERNEL_ERROR_NULL_POINTER);
	exos_mutex_lock(&_drivers_lock);
	bool done = 0;
	FOREACH(node, &_drivers)
	{
		usb_host_function_driver_node_t *driver_node = (usb_host_function_driver_node_t *)node;
		done = callback(driver_node->Driver, arg);
		if (done) break;
	}
	exos_mutex_unlock(&_drivers_lock);
	return done;
}

static void _create_device(usb_host_device_t *device, usb_host_controller_t *hc, unsigned port, usb_host_device_speed_t speed)
{
	ASSERT(device != nullptr && hc != nullptr, KERNEL_ERROR_NULL_POINTER);
	device->Controller = hc;
	list_initialize(&device->Functions);
	exos_mutex_create(&device->ControlMutex);

	device->Port = port;
	device->Speed = speed;
	device->Address = 0;

	device->Vendor = device->Product = 0;
	device->State = USB_HOST_DEVICE_CREATED;

	// setup control pipe with initial 8 byte max transfer size
	usb_host_pipe_t *pipe = &device->ControlPipe;
	*pipe = (usb_host_pipe_t) { .Device = device, .EndpointType = USB_TT_CONTROL, .MaxPacketSize = 8 };
}

usb_host_device_t *usb_host_create_root_device(usb_host_controller_t *hc, unsigned port, usb_host_device_speed_t speed)
{
	ASSERT(hc != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(port < hc->RootHubPorts, KERNEL_ERROR_KERNEL_PANIC);
	ASSERT(hc->Devices != nullptr, KERNEL_ERROR_NULL_POINTER);
	usb_host_device_t *device = &hc->Devices[port];
	_create_device(device, hc, port, speed);

	const usb_host_controller_driver_t *hcd = hc->Driver;
	ASSERT(hcd != nullptr && hcd->CreateDevice != nullptr, KERNEL_ERROR_NULL_POINTER);
	bool done = hcd->CreateDevice(hc, device, port, speed);
	return done ? device: nullptr;
}

bool usb_host_create_child_device(usb_host_device_t *hub, usb_host_device_t *child, unsigned port, usb_host_device_speed_t speed)
{
	ASSERT(hub != nullptr && child != nullptr, KERNEL_ERROR_NULL_POINTER);
	usb_host_controller_t *hc = hub->Controller;
	ASSERT(hc != nullptr, KERNEL_ERROR_NULL_POINTER);
	_create_device(child, hc, port, speed);

	const usb_host_controller_driver_t *hcd = hc->Driver;
	ASSERT(hcd != nullptr && hcd->CreateDevice != nullptr, KERNEL_ERROR_NULL_POINTER);
	bool done = hcd->CreateDevice(hc, child, port, speed);
	return done;
}

static void _destroy_device(usb_host_device_t *device)
{
	ASSERT(device != nullptr, KERNEL_ERROR_NULL_POINTER);
	FOREACH(node, &device->Functions)
	{
		usb_host_function_t *func = (usb_host_function_t *)node;
		usb_host_destroy_function(func);
	}
}

void usb_host_destroy_device(usb_host_device_t *device)
{
	ASSERT(device != nullptr, KERNEL_ERROR_NULL_POINTER);
	usb_host_controller_t *hc = device->Controller;
	ASSERT(hc != nullptr, KERNEL_ERROR_NULL_POINTER);

	if (device->State != USB_HOST_DEVICE_CREATED)
	{
		const usb_host_controller_driver_t *hcd = hc->Driver;
		ASSERT(hcd != nullptr && hcd->DestroyDevice != nullptr, KERNEL_ERROR_NULL_POINTER);
		hcd->DestroyDevice(hc, device);

		_destroy_device(device);
	}
}

void usb_host_create_function(usb_host_function_t *func, usb_host_device_t *device, const usb_host_function_driver_t *driver)
{
	ASSERT(func != nullptr && device != nullptr && driver != nullptr, KERNEL_ERROR_NULL_POINTER);
	*func = (usb_host_function_t) {
		.Device = device, .Driver = driver };
}

void usb_host_destroy_function(usb_host_function_t *func)
{
	ASSERT(func != nullptr, KERNEL_ERROR_NULL_POINTER);
	const usb_host_function_driver_t *driver = func->Driver;
    driver->Stop(func);
}

void usb_host_init_pipe_from_descriptor(usb_host_device_t *device, usb_host_pipe_t *pipe, usb_endpoint_descriptor_t *ep_desc)
{
	ASSERT(device != nullptr && pipe != nullptr && ep_desc != nullptr, KERNEL_ERROR_NULL_POINTER);
	*pipe = (usb_host_pipe_t) {
		.Device = device,
		.EndpointType = ep_desc->AttributesBits.TransferType,
		.Direction = ep_desc->AddressBits.Input ? USB_DEVICE_TO_HOST : USB_HOST_TO_DEVICE,
		.MaxPacketSize = USB16TOH(ep_desc->MaxPacketSize),
		.EndpointNumber = ep_desc->AddressBits.EndpointNumber,
		.InterruptInterval = ep_desc->Interval };
}

bool usb_host_start_pipe(usb_host_pipe_t *pipe)
{
	ASSERT(pipe != nullptr, KERNEL_ERROR_NULL_POINTER);
	usb_host_device_t *device = pipe->Device;
	ASSERT(device != nullptr, KERNEL_ERROR_NULL_POINTER);
	exos_mutex_lock(&device->ControlMutex);
	usb_host_controller_t *hc = device->Controller;
	ASSERT(hc != nullptr, KERNEL_ERROR_NULL_POINTER);
	const usb_host_controller_driver_t *hcd = hc->Driver;
	ASSERT(hcd != nullptr, KERNEL_ERROR_NULL_POINTER);
	bool done = hcd->StartPipe(hc, pipe);
	exos_mutex_unlock(&device->ControlMutex);
	return done;
}

bool usb_host_stop_pipe(usb_host_pipe_t *pipe)
{
	ASSERT(pipe != nullptr, KERNEL_ERROR_NULL_POINTER);
	usb_host_device_t *device = pipe->Device;
	ASSERT(device != nullptr, KERNEL_ERROR_NULL_POINTER);
	exos_mutex_lock(&device->ControlMutex);
	usb_host_controller_t *hc = device->Controller;
	ASSERT(hc != nullptr, KERNEL_ERROR_NULL_POINTER);
	const usb_host_controller_driver_t *hcd = hc->Driver;
	ASSERT(hcd != nullptr, KERNEL_ERROR_NULL_POINTER);
	bool done = hcd->StopPipe(hc, pipe);
	exos_mutex_unlock(&device->ControlMutex);
	return done;	
}

int usb_host_do_transfer(usb_host_pipe_t *pipe, void *data, unsigned length, unsigned timeout)
{
	ASSERT(pipe != nullptr, KERNEL_ERROR_NULL_POINTER);
	usb_host_device_t *device = pipe->Device;
	ASSERT(device != nullptr, KERNEL_ERROR_NULL_POINTER);
	usb_host_controller_t *hc = device->Controller;
	ASSERT(hc != nullptr, KERNEL_ERROR_NULL_POINTER);
	const usb_host_controller_driver_t *hcd = hc->Driver;

	int done = 0;
	usb_request_buffer_t urb;
	usb_host_urb_create(&urb, pipe);
	if (hcd->BeginTransfer(hc, &urb, data, length))
	{
		done = hcd->EndTransfer(hc, &urb, timeout);
	}
	return done;
}

bool usb_host_begin_transfer(usb_request_buffer_t *urb, void *data, unsigned length)
{
	ASSERT(urb != nullptr && urb->Pipe != nullptr, KERNEL_ERROR_NULL_POINTER);
	usb_host_device_t *device = urb->Pipe->Device;
	ASSERT(device != nullptr, KERNEL_ERROR_NULL_POINTER);
	usb_host_controller_t *hc = device->Controller;
	ASSERT(hc != nullptr, KERNEL_ERROR_NULL_POINTER);
	const usb_host_controller_driver_t *hcd = hc->Driver;

	bool done = hcd->BeginTransfer(hc, urb, data, length);
	return done;
}

int usb_host_end_transfer(usb_request_buffer_t *urb, unsigned timeout)
{
	ASSERT(urb != nullptr && urb->Pipe != nullptr, KERNEL_ERROR_NULL_POINTER);
	usb_host_device_t *device = urb->Pipe->Device;
	ASSERT(device != nullptr, KERNEL_ERROR_NULL_POINTER);
	usb_host_controller_t *hc = device->Controller;
	ASSERT(hc != nullptr, KERNEL_ERROR_NULL_POINTER);
	const usb_host_controller_driver_t *hcd = hc->Driver;

	int done = hcd->EndTransfer(hc, urb, timeout);
	return done;
}

bool usb_host_ctrl_setup(usb_host_device_t *device, const usb_request_t *request, void *data, unsigned length)
{
	ASSERT(request != nullptr, KERNEL_ERROR_NULL_POINTER);
	if (device == nullptr || device->State != USB_HOST_DEVICE_ATTACHED) 
		return false;

	exos_mutex_lock(&device->ControlMutex);
	usb_host_controller_t *hc = device->Controller;
	ASSERT(hc != nullptr, KERNEL_ERROR_NULL_POINTER);
	const usb_host_controller_driver_t *hcd = hc->Driver;

	device->ControlBuffer = *request;
	bool done = (length != 0 &&
		(request->RequestType & USB_REQTYPE_DIRECTION_MASK) == USB_REQTYPE_DEVICE_TO_HOST) ?
		hcd->CtrlSetupRead(device, &device->ControlBuffer, sizeof(usb_request_t), data, length) :
		hcd->CtrlSetupWrite(device, &device->ControlBuffer, sizeof(usb_request_t), data, length);
	exos_mutex_unlock(&device->ControlMutex);
    return done;
}

bool usb_host_read_device_descriptor(usb_host_device_t *device, unsigned char desc_type, unsigned char desc_index, void *data, unsigned short length)
{
	usb_request_t req = (usb_request_t) {
		.RequestType = USB_REQTYPE_DEVICE_TO_HOST | USB_REQTYPE_RECIPIENT_DEVICE,
		.RequestCode = USB_REQUEST_GET_DESCRIPTOR,
		.Value = (desc_type << 8) | desc_index, .Index = 0, .Length = length };
	return usb_host_ctrl_setup(device, &req, data, length);
}

bool usb_host_read_string_descriptor(usb_host_device_t *device, unsigned short lang_id, unsigned char str_index, void *data, unsigned short length)
{
	usb_request_t req = (usb_request_t) {
		.RequestType = USB_REQTYPE_DEVICE_TO_HOST | USB_REQTYPE_RECIPIENT_DEVICE,
		.RequestCode = USB_REQUEST_GET_DESCRIPTOR,
		.Value = (USB_DESCRIPTOR_TYPE_STRING << 8) | str_index, .Index = lang_id, .Length = length };
	return usb_host_ctrl_setup(device, &req, data, length);
}

bool usb_host_read_if_descriptor(usb_host_device_t *device, unsigned short interface, unsigned char desc_type, unsigned char desc_index, void *data, unsigned short length)
{
	usb_request_t req = (usb_request_t) {
		.RequestType = USB_REQTYPE_DEVICE_TO_HOST | USB_REQTYPE_RECIPIENT_INTERFACE,
		.RequestCode = USB_REQUEST_GET_DESCRIPTOR,
		.Value = (desc_type << 8) | desc_index, .Index = interface, .Length = length };
	return usb_host_ctrl_setup(device, &req, data, length);
}

bool usb_host_set_address(usb_host_device_t *device, unsigned char addr)
{
	usb_request_t req = (usb_request_t) {
		.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_RECIPIENT_DEVICE,
		.RequestCode = USB_REQUEST_SET_ADDRESS,
		.Value = addr, .Index = 0, .Length = 0 };
	return usb_host_ctrl_setup(device, &req, nullptr, 0);
}

bool usb_host_set_interface(usb_host_device_t *device, unsigned short interface, unsigned short alternate_setting)
{
	usb_request_t req = (usb_request_t) {
		.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_RECIPIENT_INTERFACE,
		.RequestCode = USB_REQUEST_SET_INTERFACE,
		.Value = alternate_setting, .Index = interface, .Length = 0 };
	return usb_host_ctrl_setup(device, &req, nullptr, 0);
}

void usb_host_urb_create(usb_request_buffer_t *urb, usb_host_pipe_t *pipe)
{
	ASSERT(urb != nullptr && pipe != nullptr, KERNEL_ERROR_NULL_POINTER);
	urb->Pipe = pipe;
	exos_event_create(&urb->Event, EXOS_EVENTF_AUTORESET);
	urb->Status = URB_STATUS_EMPTY;
}







