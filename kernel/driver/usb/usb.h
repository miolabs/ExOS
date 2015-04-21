#ifndef USB_H
#define USB_H

#include <kernel/types.h>

// define compiler storage attribute for usb buffers
#define __usb __attribute__((__section__(".usb")))

typedef enum
{
	USB_DESCRIPTOR_TYPE_ANY = 0,
	USB_DESCRIPTOR_TYPE_DEVICE = 1,
	USB_DESCRIPTOR_TYPE_CONFIGURATION = 2,
	USB_DESCRIPTOR_TYPE_STRING = 3,
	USB_DESCRIPTOR_TYPE_INTERFACE = 4,
	USB_DESCRIPTOR_TYPE_ENDPOINT = 5,
	USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER = 6,	
	USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION = 11,
} USB_DESCRIPTOR_TYPE;

// Control RequestType
typedef enum
{
	USB_REQTYPE_DEVICE_TO_HOST = 0x80,
	USB_REQTYPE_HOST_TO_DEVICE = 0x00,
	USB_REQTYPE_DIRECTION_MASK = 0x80,
} USB_REQTYPE_DIRECTION;

typedef enum
{
	USB_REQTYPE_STANDARD = 0x00,
	USB_REQTYPE_CLASS = 0x20,
	USB_REQTYPE_VENDOR = 0x40,
	USB_REQTYPE_RESERVED = 0x60,
	USB_REQTYPE_MASK = 0x60,
} USB_REQTYPE;

typedef enum
{
	USB_REQTYPE_RECIPIENT_DEVICE = 0x00,
	USB_REQTYPE_RECIPIENT_INTERFACE = 0x01,
	USB_REQTYPE_RECIPIENT_ENDPOINT = 0x02,
	USB_REQTYPE_RECIPIENT_MASK = 0x1F,
} USB_REQUEST_RECIPIENT;

// USB Standard Requests
typedef enum
{
	USB_REQUEST_GET_STATUS = 0,
	USB_REQUEST_CLEAR_FEATURE = 1,
	USB_REQUEST_SET_FEATURE = 3,
	USB_REQUEST_SET_ADDRESS = 5,
	USB_REQUEST_GET_DESCRIPTOR = 6,
	USB_REQUEST_GET_CONFIGURATION = 8,
	USB_REQUEST_SET_CONFIGURATION = 9,
	USB_REQUEST_GET_INTERFACE = 10,
	USB_REQUEST_SET_INTERFACE = 11,
} USB_REQUEST_CODE;

// USB_Features
typedef enum
{
	USB_FEATURE_ENDPOINT_HALT = 0,	
	USB_FEATURE_DEVICE_REMOTE_WAKEUP = 1,
	USB_FEATURE_DEVICE_TEST_MODE = 2,
} USB_FEATURE;

typedef struct _USB_REQUEST_BUF
{
	unsigned char RequestType;
	unsigned char RequestCode;
	unsigned short Value;
	unsigned short Index;
	unsigned short Length;
} USB_REQUEST;

#define USB_REQ_INDEX_EP_INPUT 0x80


// descriptors info
/////////////////////////

typedef struct __attribute__((__packed__))
{
	unsigned char Bytes[2];
} USB16;

typedef struct __attribute__((__packed__))
{
	unsigned char Bytes[4];
} USB32;

#define USB16TOH(u) (((u).Bytes[1] << 8) | ((u).Bytes[0]))
#define HTOUSB16(h) (USB16){ (unsigned char)((h) & 0xFF), (unsigned char)((h) >> 8) }

typedef struct __attribute__((__packed__))
{
	unsigned char Length;
	unsigned char DescriptorType;
} USB_DESCRIPTOR_HEADER;

typedef struct __attribute__((__packed__))
{
	USB_DESCRIPTOR_HEADER Header;
	USB16 USBVersion;
	unsigned char DeviceClass;
	unsigned char DeviceSubClass;
	unsigned char DeviceProtocol;
	unsigned char MaxPacketSize;	// for endpoint 0
	USB16 VendorId;
	USB16 ProductId;
	USB16 DeviceVersion;
	unsigned char ManufacturerIndex;
	unsigned char ProductIndex;
	unsigned char SerialNumberIndex;
	unsigned char NumConfigurations;
} USB_DEVICE_DESCRIPTOR;

typedef struct __attribute__((__packed__))
{
	USB_DESCRIPTOR_HEADER Header;
	USB16 TotalLength;
	unsigned char NumInterfaces;
	unsigned char ConfigurationValue;
	unsigned char ConfigurationIndex;
	union __attribute__((__packed__))
	{
		unsigned char Attributes;
		struct __attribute__((__packed__))
		{
			unsigned :5;
			unsigned RemoteWakeup:1;
			unsigned SelfPowered:1;
			unsigned Res1_BusPowered:1;
		} AttributesBits;
	};
	unsigned char MaxPower;	// In 2mA units
} USB_CONFIGURATION_DESCRIPTOR;

typedef struct __attribute__((__packed__))
{
	USB_DESCRIPTOR_HEADER Header;
	unsigned char InterfaceNumber;
	unsigned char AlternateSetting;
	unsigned char NumEndpoints;
	unsigned char InterfaceClass;
	unsigned char InterfaceSubClass;
	unsigned char Protocol;
	unsigned char InterfaceNameIndex;	
} USB_INTERFACE_DESCRIPTOR;

typedef enum
{
	USB_TT_CONTROL = 0,
	USB_TT_ISO = 1,
	USB_TT_BULK = 2,
	USB_TT_INTERRUPT = 3,
} USB_TRANSFERTYPE;

typedef enum
{
	USB_HOST_TO_DEVICE = 0,
	USB_DEVICE_TO_HOST = 1,
} USB_DIRECTION;

typedef struct __attribute__((__packed__))
{
	USB_DESCRIPTOR_HEADER Header;
	union __attribute__((__packed__))
	{
		unsigned char Address;
		struct __attribute__((__packed__))
		{
			unsigned EndpointNumber:4;
			unsigned :3;
			unsigned Input:1;
		} AddressBits;
	};
	union __attribute__((__packed__))
	{
		unsigned char Attributes;
		struct __attribute__((__packed__))
		{
			unsigned TransferType:2;
			unsigned IsoSyncType:2;
			unsigned IsoUsageType:2;
			unsigned :2;
		} AttributesBits;
	};
	USB16 MaxPacketSize;
	unsigned char Interval;
} USB_ENDPOINT_DESCRIPTOR;

typedef struct __attribute__((__packed__))
{
	USB_DESCRIPTOR_HEADER Header;
	USB16 String[0];
} USB_STRING_DESCRIPTOR;

typedef struct __attribute__((__packed__))
{
	USB_DESCRIPTOR_HEADER Header;
	unsigned char FirstInterface;
	unsigned char InterfaceCount;
	unsigned char FunctionClass;
	unsigned char FunctionSubClass;
	unsigned char FunctionProtocol;
	unsigned char FunctionIndex;
} USB_INTERFACE_ASSOCIATION_DESCRIPTOR;

typedef enum 
{
	USB_USB_1_0 = 0x0100,
	USB_USB_1_1 = 0x0110,
	USB_USB_2_0 = 0x0200,
} USB_USB_SPEC;

typedef enum
{
	USB_CLASS_PER_INTERFACE = 0,	// When this class is present in Device Desc, each interface defines its own class
	USB_CLASS_AUDIO = 1,
    USB_CLASS_COMM = 2,
    USB_CLASS_HID = 3,
    USB_CLASS_STILL_IMAGE = 6, 
    USB_CLASS_PRINTER = 7, 
    USB_CLASS_MASS_STORAGE = 8, 
	USB_CLASS_HUB = 9,
    USB_CLASS_DATA = 10, 
	USB_CLASS_VIDEO = 14,
	USB_CLASS_MISC = 0xEF, // used, i.e., in devices of classes using Interface Association
	USB_CLASS_CUSTOM = 0xFF,
} USB_CLASS_CODE;

typedef enum
{
	USB_DEVICE_SUBCLASS_COMMON = 2,	// used with USB_CLASS_MISC for Interface Association
} USB_SUBCLASS_CODE;

typedef enum
{
	USB_DEVICE_PROTOCOL_IAD = 1, // Interface Association Descriptor
} USB_PROTOCOL_CODE;

// bmAttributes in Configuration Descriptors
#define USB_CONFIG_POWERED_MASK                0xC0
#define USB_CONFIG_BUS_POWERED                 0x80
#define USB_CONFIG_SELF_POWERED                0x40
#define USB_CONFIG_REMOTE_WAKEUP               0x20

typedef enum
{
	USB_STATUS_SELF_POWERED = 0x01,
	USB_STATUS_REMOTE_WAKEUP  = 0x02,
} USB_STATUS;

// prototypes
int usb_parse_conf(USB_CONFIGURATION_DESCRIPTOR *conf_desc, USB_DESCRIPTOR_TYPE desc_type, USB_DESCRIPTOR_HEADER **current);
int usb_desc2str(USB_DESCRIPTOR_HEADER *str_desc, unsigned char *dest, int max_length);

#endif // OHCI_USB_H
