#ifndef SCSI_SCSI_H
#define SCSI_SCSI_H

typedef struct
{
	unsigned char Byte[4];
} SCSI32;

#define HTOSCSI32(w) (SCSI32){(unsigned char)((w)>>24), (unsigned char)((w)>>16), (unsigned char)((w)>>8), (unsigned char)(w)}
#define SCSI32TOH(s) (((s).Byte[0]<<24) + ((s).Byte[1]<<16) + ((s).Byte[2]<<8) + ((s).Byte[3]))

typedef struct
{
	unsigned char Byte[2];
} SCSI16;

#define HTOSCSI16(w) (SCSI16){(unsigned char)((w)>>8), (unsigned char)(w)}
#define SCSI16TOH(s) (((s).Byte[0]<<8) + ((s).Byte[1]))
 
typedef enum
{
	// std commands
	SCSI_CMD_TEST_UNIT_READY = 0,
	SCSI_CMD_REQUEST_SENSE = 0x03,
	SCSI_CMD_FORMAT_UNIT = 0x04,
	SCSI_CMD_READ6 = 0x08,
	SCSI_CMD_WRITE6 = 0x0A,
	SCSI_CMD_INQUIRY = 0x12,
	SCSI_CMD_MODE_SENSE6 = 0x1A,
	SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL = 0x1E,
	SCSI_CMD_READ_CAPACITY_10 = 0x25,
	SCSI_CMD_READ10 = 0x28,
	SCSI_CMD_WRITE10 = 0x2A,

	// MMC commands
	SCSI_MMC_CMD_READ_FORMAT_CAPACITIES = 0x23,
} SCSI_CMD_CODE;

typedef union __attribute__((__packed__))
{
	unsigned char Byte[6];
	struct  __attribute__((__packed__))
	{
		unsigned char OpCode;
		struct __attribute__((__packed__))
		{
			unsigned :5;
			unsigned LUN:3;
		};
		unsigned char Reserved2;
		SCSI16 AllocationLength;
		unsigned char Control;
	};
} SCSI_CMD6;

typedef union __attribute__((__packed__))
{
	unsigned char Byte[10];
	struct  __attribute__((__packed__))
	{
		unsigned char OpCode;
		struct __attribute__((__packed__))
		{
			unsigned :5;
			unsigned LUN:3;
		};
		SCSI32 LBA;
		unsigned char Reserved7;
		SCSI16 TransferLength;
		unsigned char Control;
	};
} SCSI_CMD10;

typedef union __attribute__((__packed__))
{
	unsigned char Value;
	struct __attribute__((__packed__))
	{
		unsigned RelAdr:1;
		unsigned :2;
		unsigned FUA:1;
		unsigned DPO:1;
		unsigned LUN:3;
	};
} SCSI_READ_FLAGS;

typedef struct __attribute__((__packed__))
{
	unsigned char OpCode;	// 0x28
	SCSI_READ_FLAGS Flags;
	SCSI32 LBA;
	unsigned char Reserved;
	SCSI16 TransferLength;
	unsigned char Control;
} SCSI_READ10;

typedef union __attribute__((__packed__))
{
	unsigned char Value;
	struct __attribute__((__packed__))
	{
		unsigned RelAdr:1;
		unsigned :1;
		unsigned EBP:1;
		unsigned FUA:1;
		unsigned DPO:1;
		unsigned LUN:3;
	};
} SCSI_WRITE_FLAGS;

typedef struct __attribute__((__packed__))
{
	unsigned char OpCode;	// 0x2A
	SCSI_WRITE_FLAGS Flags;
	SCSI32 LBA;
	unsigned char Reserved;
	SCSI16 TransferLength;
	unsigned char Control;
} SCSI_WRITE10;

typedef enum
{
	SCSI_SENSE_RESPONSE_CURRENT_FIXED = 0x70,
	SCSI_SENSE_RESPONSE_DEFERRED_FIXED = 0x71,
	SCSI_SENSE_RESPONSE_CURRENT_DESCRIPTOR = 0x72,
	SCSI_SENSE_RESPONSE_DEFERRED_DESCRIPTOR = 0x73,
	SCSI_SENSE_RESPONSE_VENDOR_SPECIFIC = 0x7F,
} SCSI_SENSE_RESPONSE_CODE;

typedef enum
{
	SCSI_SENSE_NO_SENSE = 0x0,
	SCSI_SENSE_RECOVERED_ERROR = 0x1,
	SCSI_SENSE_NOT_READY = 0x2,
	SCSI_SENSE_MEDIUM_ERROR = 0x3,
	SCSI_SENSE_HARDWARE_ERROR = 0x4,
	SCSI_SENSE_ILLEGAL_REQUEST = 0x5,
	SCSI_SENSE_UNIT_ATTENTION = 0x6,
	SCSI_SENSE_DATA_PROTECT = 0x7,
	SCSI_SENSE_BLANK_CHECK = 0x8,
	SCSI_SENSE_VENDOR_SPECIFIC = 0x9,
	SCSI_SENSE_COPY_ABORTED = 0xA,
	SCSI_SENSE_ABORTED_COMMAND = 0xB,
	SCSI_SENSE_OBSOLETE = 0xC,
	SCSI_SENSE_VOLUME_OVERFLOW = 0xD,
   	SCSI_SENSE_MISCOMPARE = 0xE,
	SCSI_SENSE_RESERVED = 0xF,
} SCSI_SENSE_CODE;

typedef struct __attribute__((__packed__))
{
	struct __attribute__((__packed__))
	{
		SCSI_SENSE_RESPONSE_CODE ResponseCode:7;
		unsigned :1;
	};
	struct __attribute__((__packed__))
	{
		SCSI_SENSE_CODE SenseKey:4;
		unsigned :4;
	};
	unsigned char ASC;	// AdditionalSenseCode
	unsigned char ASCQ;	// AdditionalSenseCodeQualifier;
	unsigned char Reserved[3];
	unsigned char AdditionalSenseLength;
	unsigned char AdditionalSenseBytes[0];
} SCSI_SENSE_INFO; // sizeof should be 8 + AdditionalSenseLength

typedef struct __attribute__((__packed__))
{
	SCSI32 LBA;
	SCSI32 BlockSize;
} SCSI_CAPACITY_DATA; // sizeof should be 8

typedef enum
{
	SCSI_DEVICE_DIRECT_ACCESS = 0x00,
	SCSI_DEVICE_SEQUENTIAL_ACCESS = 0x01,
	SCSI_DEVICE_PRINTER = 0x02,
    SCSI_DEVICE_PROCESSOR = 0x03,
	SCSI_DEVICE_WRITE_ONCE = 0x04,
	SCSI_DEVICE_CDROM = 0x05,
	SCSI_DEVICE_SCANNER = 0x06,
    SCSI_DEVICE_OPTICAL_MEMORY = 0x07,
    SCSI_DEVICE_MEDIUM_CHANGER = 0x08,
    SCSI_DEVICE_COMM = 0x09,
    SCSI_DEVICE_STORAGE_ARRAY = 0x0C,
    SCSI_DEVICE_ENCLOSURE_SERVICES = 0x0D,
    SCSI_DEVICE_SIMPLIFIED_DIRECT_ACCESS = 0x0E,
    SCSI_DEVICE_OPTICAL_CARD_READER = 0x0F,
    SCSI_DEVICE_UNKNOWN = 0x1F,
} SCSI_DEVICE_TYPE;

typedef struct __attribute__((__packed__))
{
	struct __attribute__((__packed__))
	{
		SCSI_DEVICE_TYPE DeviceType:5;
		unsigned Qualifier:3;
	} Peripheral;
	struct __attribute__((__packed__))
	{
		unsigned :7;
		unsigned RMB:1;
	} DeviceType;
	struct __attribute__((__packed__))
	{
		unsigned ANSI:3;
		unsigned ECMA:3;
		unsigned ISO:1;
	} Version;
	struct __attribute__((__packed__))
	{
		unsigned DataFormat:4;
		unsigned Reserved:2;
		unsigned TermIOP:1;
		unsigned AEC:1;
	} Response;
	unsigned char AdditionalLength;
	unsigned char Reserved5;
	unsigned char Reserved6;
	unsigned char Reserved7;
	unsigned char Vendor[8];
	unsigned char Product[16];
	unsigned char ProductRevision[4];
} SCSI_INQUIRY_DATA;

typedef enum
{
	SCSI_PAGE_SUPPORTED_PAGES = 0,
} SCSI_PAGE_CODE;

typedef struct
{
	SCSI_PAGE_CODE PageCode:8;
	unsigned char PageCodeSpecific;
	SCSI16 PageLength;
	unsigned char Parameters[0];
} SCSI_DIAG_PAGE;

// prototypes
void scsi_fill_read10(SCSI_READ10 *cmd, int lun, unsigned long lba, unsigned short length);


#endif // SCSI_SCSI_H
