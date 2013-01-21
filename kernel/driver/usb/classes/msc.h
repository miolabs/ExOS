#ifndef USB_CLASS_MSC_H
#define USB_CLASS_MSC_H

#include <usb/usb.h>

// Additional USB declarations for Mass Storage Class
typedef enum
{
	USB_MSC_SUBCLASS_RBC = 1,	// Reduced Block Commands (RBC) T10 Project 1240-D
	USB_MSC_SUBCLASS_MMC5 = 2,	// ATAPI
	USB_MSC_SUBCLASS_UFI = 4,	// USB Floppy Interface
	USB_MSC_SUBCLASS_SCSI = 6,
	USB_MSC_SUBCLASS_LOCKABLE = 7,
	USB_MSC_SUBCLASS_IEEE1667 = 8,
	USB_MSC_SUBCLASS_VENDOR_SPECIFIC = 0xFF,
} USB_MSC_SUBCLASS;

typedef enum
{
	USB_MSC_PROTOCOL_CBI = 0,
	USB_MSC_PROTOCOL_CBI_NOCC = 1,
	USB_MSC_PROTOCOL_BBB = 0x50,
	USB_MSC_PROTOCOL_VENDOR_SPECIFIC = 0xFF,
} USB_MSC_PROTOCOL;

typedef enum
{
	USB_MSC_REQ_ADSC = 0,	// Accept Device-Specific Command
	USB_MSC_REQ_GML = 0xFE,		// Get Max LUN
	USB_MSC_REQ_BOMSR = 0xFF,	// Bulk-Only Mass Storage Reset
} USB_MSC_REQUEST_CODE;

#define	USB_MSC_MAX_CDB_LENGTH 16	// Max length of embedded SCSI cmd length

typedef struct __attribute__((__packed__))
{
	unsigned long Signature;
	unsigned long Tag;
	unsigned long DataTransferLength;
	union __attribute__((__packed__))
	{
		unsigned char Flags;
		struct __attribute__((__packed__))
		{
			unsigned Reserved:7;
			unsigned Input:1;
		} FlagsBits;
	};
	unsigned char LUN;
	unsigned char CDBLength;
	unsigned char CDB[USB_MSC_MAX_CDB_LENGTH];
} USB_MSC_CBW;	// sizeof should be 31 (0x1F)

#define	USB_MSC_CBW_SIGNATURE	0x43425355

typedef struct __attribute__((__packed__))
{
	unsigned long Signature;
	unsigned long Tag;
	unsigned long DataResidue;
	unsigned char Status;
} USB_MSC_CSW;	// sizeof should be 13 (0x0D)

#define  USB_MSC_CSW_SIGNATURE	0x53425355

typedef enum
{
	USB_MSC_STA_PASSED = 0,
	USB_MSC_STA_FAILED,
	USB_MSC_STA_PHASE_ERROR,
	USB_MSC_STA_RES3,
	USB_MSC_STA_RES4,
} USB_MSC_STATUS;




#endif // USB_CLASS_MSC_H
