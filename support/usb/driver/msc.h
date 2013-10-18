#ifndef USB_DRIVER_MSC_H
#define USB_DRIVER_MSC_H

#include <usb/classes/msc.h>
#include <usb/classes/scsi.h>
#include <usb/host.h>
#include <kernel/tree.h>

typedef struct
{
	USB_HOST_FUNCTION;
	USB_HOST_PIPE BulkInputPipe;
	USB_HOST_PIPE BulkOutputPipe;
//	EXOS_TREE_DEVICE KernelDevice;
	unsigned long NumBlocks;
	unsigned short BlockSize;
   	unsigned char Interface;
	unsigned char Reserved;

	// NOTE: data has no align requirements
	USB_MSC_CBW CBW;
	USB_MSC_CSW CSW;
	// FIXME: to be removed from here
	SCSI_INQUIRY_DATA InquiryData;	
	unsigned char Buffer[512];
} MSC_FUNCTION;

// prototypes
void usbd_msc_initialize();
int usbd_msc_get_max_lun(MSC_FUNCTION *func, int *max_lun);
int usbd_msc_boms_reset(MSC_FUNCTION *func);
int usbd_msc_test_unit_ready(MSC_FUNCTION *func);
SCSI_SENSE_INFO *usbd_msc_get_sense_info(MSC_FUNCTION *func);
SCSI_CAPACITY_DATA *usbd_msc_read_capacity(MSC_FUNCTION *func);
SCSI_INQUIRY_DATA *usbd_msc_inquiry(MSC_FUNCTION *func);
void *usbd_msc_read_block(MSC_FUNCTION *func, unsigned long lba);

#endif // USB_DRIVER_MSC_H
