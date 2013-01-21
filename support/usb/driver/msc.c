#include "msc.h"
#include <usb/enumerate.h>
#include <usb/classes/scsi.h>

static USB_HOST_FUNCTION *_check_interface(USB_HOST_DEVICE *device, USB_CONFIGURATION_DESCRIPTOR *conf_desc, USB_DESCRIPTOR_HEADER *fn_desc);
static void _start(USB_HOST_FUNCTION *func);

static MSC_FUNCTION _func __usb;	// currently, a single instance is supported
static const USB_HOST_FUNCTION_DRIVER _driver = { _check_interface, _start, NULL };
static USB_HOST_FUNCTION_DRIVER_NODE _driver_node;
static unsigned char _tag;

static void _reset_recovery(MSC_FUNCTION *func);

void usbd_msc_initialize()
{
	_driver_node = (USB_HOST_FUNCTION_DRIVER_NODE) { .Driver = &_driver };
	usb_host_driver_register(&_driver_node);
}

static USB_HOST_FUNCTION *_check_interface(USB_HOST_DEVICE *device, USB_CONFIGURATION_DESCRIPTOR *conf_desc, USB_DESCRIPTOR_HEADER *fn_desc)
{
	if (fn_desc->DescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE)
	{
		USB_INTERFACE_DESCRIPTOR *if_desc = (USB_INTERFACE_DESCRIPTOR *)fn_desc;

		if (if_desc->InterfaceClass == USB_CLASS_MASS_STORAGE
			&& if_desc->InterfaceSubClass == USB_MSC_SUBCLASS_SCSI
			&& if_desc->Protocol == USB_MSC_PROTOCOL_BBB)
		{
			MSC_FUNCTION *func = &_func;
			usb_host_create_function((USB_HOST_FUNCTION *)func, device, &_driver);

			USB_ENDPOINT_DESCRIPTOR *ep_desc;
			ep_desc = usb_enumerate_find_endpoint_descriptor(conf_desc, if_desc, USB_TT_BULK, USB_DEVICE_TO_HOST, 0);
			if (!ep_desc) return NULL;
			usb_host_init_pipe_from_descriptor(device, &func->BulkInputPipe, ep_desc);
			
			ep_desc = usb_enumerate_find_endpoint_descriptor(conf_desc, if_desc, USB_TT_BULK, USB_HOST_TO_DEVICE, 0);
			if (!ep_desc) return NULL;
			usb_host_init_pipe_from_descriptor(device, &func->BulkOutputPipe, ep_desc);
				
			func->Interface = if_desc->InterfaceNumber;
			return (USB_HOST_FUNCTION *)func;
		}
	}
	return NULL;
}

static void _start(USB_HOST_FUNCTION *usb_func)
{
	MSC_FUNCTION *func = (MSC_FUNCTION *)usb_func;
	usb_host_start_pipe(&func->BulkInputPipe);
	usb_host_start_pipe(&func->BulkOutputPipe);

	int max_lun;
	int done = usbd_msc_get_max_lun(func, &max_lun);
	if (done)
	{
		// NOTE: actual max_lun value is ignored, and used always LUN 0
		SCSI_INQUIRY_DATA *inquiry = usbd_msc_inquiry(func);
		if (inquiry != NULL)
		{
			func->InquiryData = *inquiry;

			for(int retry = 0; retry < 25; retry++)
			{
				done = usbd_msc_test_unit_ready(func);
				if (done) break;
				usbd_msc_get_sense_info(func);
			}
	
			if (done)
			{
				SCSI_CAPACITY_DATA *capacity = usbd_msc_read_capacity(func);
				if (capacity != NULL)
				{
					func->NumBlocks = SCSI32TOH(capacity->LBA);
					func->BlockSize = SCSI32TOH(capacity->BlockSize);
					long long size = func->NumBlocks * func->BlockSize;
					if (size == 0 || func->BlockSize != 512) done = 0;	// FIXME: invalid size!
					
					func->KernelDevice = (EXOS_TREE_DEVICE) {
						.Name = "usbdisk",
						.DeviceType = EXOS_TREE_DEVICE_BLOCK,
						.Device = (COMM_DEVICE *)func, // FIXME
						.Unit = 0 };
					
                    exos_tree_add_device(&func->KernelDevice);
				}
				else done = 0;
			}
		}
		else done = 0;
	}
//	return done;
}

int usbd_msc_get_max_lun(MSC_FUNCTION *func, int *max_lun)
{
	// NOTE: this function is a control request, not a scsi command
	USB_REQUEST setup = (USB_REQUEST) {
		.RequestType = USB_REQTYPE_DEVICE_TO_HOST | USB_REQTYPE_CLASS | USB_REQTYPE_RECIPIENT_INTERFACE,
		.RequestCode = USB_MSC_REQ_GML,
		.Value = 0, .Index = func->Interface, .Length = 1 };
	int done = usb_host_ctrl_setup_read(func->Device, &setup, sizeof(USB_REQUEST), func->Buffer, 1);
	*max_lun = done ? (int)func->Buffer[0] : -1;
	return done;
}

int usbd_msc_boms_reset(MSC_FUNCTION *func)
{
	USB_REQUEST setup = (USB_REQUEST) {
		.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_CLASS | USB_REQTYPE_RECIPIENT_INTERFACE,
		.RequestCode = USB_MSC_REQ_BOMSR,
		.Value = 0, .Index = func->Interface, .Length = 0 };
	int done = usb_host_ctrl_setup_read(func->Device, &setup, sizeof(USB_REQUEST), NULL, 0);
	return done;
}

void *_fill_cbw(USB_MSC_CBW *cbw, int direction, int data_length, int scsi_length)
{
	cbw->Signature = USB_MSC_CBW_SIGNATURE;
	cbw->Tag = ++_tag;
	cbw->DataTransferLength = data_length;
	cbw->Flags = 0;
	cbw->FlagsBits.Input = direction;
	cbw->LUN = 0;	// FIXME: support different LUNs
	cbw->CDBLength = scsi_length;
	// clear scsi cmd
	for(int i = 0; i < scsi_length; i++) cbw->CDB[i] = 0;
	return (void *)cbw->CDB;
}

int _do_scsi_cmd(MSC_FUNCTION *func, void *data)
{
	USB_MSC_CBW *cbw = &func->CBW;
	int done = usb_host_bulk_transfer(&func->BulkOutputPipe, cbw, sizeof(USB_MSC_CBW));
	if (done)
	{
		if (cbw->DataTransferLength != 0 && data != NULL)
		{
			if (cbw->FlagsBits.Input == 0)
			{
				// data output
				done = usb_host_bulk_transfer(&func->BulkOutputPipe, data, cbw->DataTransferLength);
			}
			else
			{
				// data input
				done = usb_host_bulk_transfer(&func->BulkInputPipe, data, cbw->DataTransferLength);
			}
			if (!done)
			{
				// FIXME: do error recovery
				return 0;
			}
		}

		USB_MSC_CSW *csw = &func->CSW;
		done = usb_host_bulk_transfer(&func->BulkInputPipe, csw, sizeof(USB_MSC_CSW));
		if (done)
		{
			// check cmd result
			switch(csw->Status)
			{
				case USB_MSC_STA_PASSED:	
					break;
				case USB_MSC_STA_PHASE_ERROR:
					_reset_recovery(func);
					done = 0;
					break;
				default:
					done = 0;
					break;
			}
		}
	}
	return done;
}

static void _reset_recovery(MSC_FUNCTION *func)
{
	USB_HOST_DEVICE *device = func->Device;
	int done = usbd_msc_boms_reset(func);
	
	if (!done)
	{
		USB_REQUEST setup;

		// clear feature HALT to bulk-in ep
		setup = (USB_REQUEST) {
			.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_RECIPIENT_ENDPOINT,
			.RequestCode = USB_REQUEST_CLEAR_FEATURE,
			.Value = USB_FEATURE_ENDPOINT_HALT, 
			.Index = func->BulkInputPipe.EndpointNumber | USB_REQ_INDEX_EP_INPUT, 
			.Length = 0 };
		done = usb_host_ctrl_setup_read(device, &setup, sizeof(USB_REQUEST), NULL, 0);

		// clear feature HALT to bulk-out ep
		setup = (USB_REQUEST) {
			.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_RECIPIENT_ENDPOINT,
			.RequestCode = USB_REQUEST_CLEAR_FEATURE,
			.Value = USB_FEATURE_ENDPOINT_HALT, 
			.Index = func->BulkOutputPipe.EndpointNumber, 
			.Length = 0 };
		done = usb_host_ctrl_setup_read(device, &setup, sizeof(USB_REQUEST), NULL, 0);
	}
}

int usbd_msc_test_unit_ready(MSC_FUNCTION *func)
{
	SCSI_CMD6 *cmd = _fill_cbw(&func->CBW, 1, 0, sizeof(SCSI_CMD6));
	cmd->OpCode = SCSI_CMD_TEST_UNIT_READY;
	int done = _do_scsi_cmd(func, NULL);
	return done;
}

SCSI_SENSE_INFO *usbd_msc_get_sense_info(MSC_FUNCTION *func)
{
	SCSI_CMD6 *cmd = _fill_cbw(&func->CBW, 1, sizeof(SCSI_SENSE_INFO), sizeof(SCSI_CMD6));
	cmd->OpCode = SCSI_CMD_REQUEST_SENSE;
	cmd->AllocationLength = HTOSCSI16(sizeof(SCSI_SENSE_INFO));

	SCSI_SENSE_INFO *sense = (SCSI_SENSE_INFO *)func->Buffer;
	int done = _do_scsi_cmd(func, sense);
	return (done ? sense : NULL);
}

SCSI_CAPACITY_DATA *usbd_msc_read_capacity(MSC_FUNCTION *func)
{
	SCSI_CMD10 *cmd = _fill_cbw(&func->CBW, 1, sizeof(SCSI_CAPACITY_DATA), sizeof(SCSI_CMD10));
	cmd->OpCode = SCSI_CMD_READ_CAPACITY_10;
	cmd->LBA = HTOSCSI32(0);

	SCSI_CAPACITY_DATA *capacity = (SCSI_CAPACITY_DATA *)func->Buffer;
	int done = _do_scsi_cmd(func, capacity);
	return (done ? capacity : NULL);
}

SCSI_INQUIRY_DATA *usbd_msc_inquiry(MSC_FUNCTION *func)
{
	SCSI_CMD6 *cmd = _fill_cbw(&func->CBW, 1, sizeof(SCSI_INQUIRY_DATA), sizeof(SCSI_CMD6));
	cmd->OpCode = SCSI_CMD_INQUIRY;
	cmd->AllocationLength = HTOSCSI16(sizeof(SCSI_INQUIRY_DATA));

	SCSI_INQUIRY_DATA *inquiry = (SCSI_INQUIRY_DATA *)func->Buffer;
	int done = _do_scsi_cmd(func, inquiry);
	return (done ? inquiry : NULL);
}

void *usbd_msc_read_block(MSC_FUNCTION *func, unsigned long lba)
{
	SCSI_READ10 *cmd = _fill_cbw(&func->CBW, 1, func->BlockSize, sizeof(SCSI_READ10));
	cmd->OpCode = SCSI_CMD_READ10;
	scsi_fill_read10(cmd, 0, lba, 1);

	int done = _do_scsi_cmd(func, func->Buffer);
	return (done ? func->Buffer : NULL);
}


