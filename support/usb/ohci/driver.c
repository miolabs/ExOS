#include "driver.h"
#include "buffers.h"
#include <kernel/fifo.h>
#include <kernel/memory.h>
#include <kernel/panic.h>

static int _ctrl_setup_read(USB_HOST_DEVICE *device, void *setup_data, int setup_length, void *in_data, int in_length);
static int _ctrl_setup_write(USB_HOST_DEVICE *device, void *setup_data, int setup_length, void *out_data, int out_length);
static int _ctrl_setup(USB_HOST_DEVICE *device, void *setup_data, int setup_length);
static int _start_pipe(USB_HOST_PIPE *pipe);
static int _bulk_transfer(USB_HOST_PIPE *pipe, void *data, int length);

const USB_HOST_CONTROLLER_DRIVER __ohci_driver = {
	_ctrl_setup_read, _ctrl_setup_write, _ctrl_setup,
	_start_pipe, _bulk_transfer,
	};

static USB_HOST_DEVICE _devices[2] __usb;	// FIXME: allow more devices (each port can generate more than one)

void ohci_driver_initialize()
{
    // initialize static buffers
	ohci_buffers_initialize();

	ohci_initialize();
}

static int _start_pipe(USB_HOST_PIPE *pipe)
{
	if (pipe == NULL || pipe->Device == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
	if (pipe->Endpoint != NULL)
		kernel_panic(KERNEL_ERROR_ALREADY_IN_USE);
	
	OHCI_SED *sed = ohci_buffers_alloc_sed();
	if (sed != NULL)
	{
		OHCI_HCED *hced = &sed->HCED;
		ohci_clear_hced(hced);
		hced->ControlBits.FunctionAddress = pipe->Device->Address;
		hced->ControlBits.Endpoint = pipe->EndpointNumber;
		hced->ControlBits.MaxPacketSize = pipe->MaxPacketSize;
		hced->ControlBits.Speed = (pipe->Device->Speed == USB_HOST_DEVICE_LOW_SPEED) ? 1 : 0;
		hced->ControlBits.Format = (pipe->EndpointType == USB_TT_ISO) ? 1 : 0;
		hced->ControlBits.Direction = (pipe->EndpointType == USB_TT_CONTROL) ? OHCI_ED_DIR_FROM_TD :
			(pipe->Direction == USB_HOST_TO_DEVICE ? OHCI_ED_DIR_OUT : OHCI_ED_DIR_IN);

		OHCI_STD *std = ohci_buffers_alloc_std();
		if (std != NULL) 
		{
			OHCI_HCTD *hctd = &std->HCTD;
			ohci_clear_hctd(hctd);
			hced->HeadTD = hctd;
			hced->TailTD = hctd;

			pipe->Endpoint = sed;
			sed->Pipe = pipe;

            ohci_pipe_add(pipe);
			return 1;
		}

		ohci_buffers_release_sed(sed);
	}
	return 0;
}

static int _bulk_transfer(USB_HOST_PIPE *pipe, void *data, int length)
{
	OHCI_TD_PID pid = pipe->Direction == USB_HOST_TO_DEVICE ? OHCI_TD_DIR_OUT : OHCI_TD_DIR_IN;
	int done = ohci_process_std(pipe, pid, OHCI_TD_TOGGLE_CARRY, data, length);
	return done;
}

USB_HOST_DEVICE *ohci_device_create(int port, USB_HOST_DEVICE_SPEED speed)
{
	// first we get a device context for the connected device
	// and configure its control ep for initialization
	USB_HOST_DEVICE *device = &_devices[port];	// FIXME
	usb_host_create_device(device, &__ohci_driver, port, speed);

	if (_start_pipe(&device->ControlPipe))
	{			
		return device;
	}
	return NULL;
}

static int _ctrl_setup_read(USB_HOST_DEVICE *device, void *setup_data, int setup_length, void *in_data, int in_length)
{
	USB_HOST_PIPE *pipe = &device->ControlPipe;
	int done = ohci_process_std(pipe, OHCI_TD_SETUP, OHCI_TD_TOGGLE_0, setup_data, setup_length);
    if (done) 
	{
        if (in_length) 
		{
            done = ohci_process_std(pipe, OHCI_TD_DIR_IN, OHCI_TD_TOGGLE_1, in_data, in_length);
        }
        if (done) 
		{
            done = ohci_process_std(pipe, OHCI_TD_DIR_OUT, OHCI_TD_TOGGLE_1, NULL, 0);
        }
    }
    return done;
}

static int _ctrl_setup_write(USB_HOST_DEVICE *device, void *setup_data, int setup_length, void *out_data, int out_length)
{
	USB_HOST_PIPE *pipe = &device->ControlPipe;
	int done = ohci_process_std(pipe, OHCI_TD_SETUP, OHCI_TD_TOGGLE_0, setup_data, setup_length);
    if (done) 
	{
        if (out_length) 
		{
            done = ohci_process_std(pipe, OHCI_TD_DIR_OUT, OHCI_TD_TOGGLE_1, out_data, out_length);
        }
        if (done) 
		{
            done = ohci_process_std(pipe, OHCI_TD_DIR_IN, OHCI_TD_TOGGLE_1, NULL, 0);
        }
    }
    return done;
}

static int _ctrl_setup(USB_HOST_DEVICE *device, void *setup_data, int setup_length)
{
	USB_HOST_PIPE *pipe = &device->ControlPipe;
	int done = ohci_process_std(pipe, OHCI_TD_SETUP, OHCI_TD_TOGGLE_0, setup_data, setup_length);
    if (done) 
	{
		done = ohci_process_std(pipe, OHCI_TD_DIR_IN, OHCI_TD_TOGGLE_1, NULL, 0);
    }
    return done;
}



