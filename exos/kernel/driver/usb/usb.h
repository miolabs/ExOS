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
} usb_descriptor_type_t;

// Control RequestType
#define	USB_REQTYPE_DIRECTION_MASK 0x80
#define USB_REQTYPE_HOST_TO_DEVICE 0
#define USB_REQTYPE_DEVICE_TO_HOST USB_REQTYPE_DIRECTION_MASK

typedef enum
{
	USB_REQTYPE_STANDARD = 0x00,
	USB_REQTYPE_CLASS = 0x20,
	USB_REQTYPE_VENDOR = 0x40,
	USB_REQTYPE_RESERVED = 0x60,
	USB_REQTYPE_MASK = 0x60,
} usb_reqtype_t;

typedef enum
{
	USB_REQTYPE_RECIPIENT_DEVICE = 0x00,
	USB_REQTYPE_RECIPIENT_INTERFACE = 0x01,
	USB_REQTYPE_RECIPIENT_ENDPOINT = 0x02,
	USB_REQTYPE_RECIPIENT_OTHER = 0x03,
	USB_REQTYPE_RECIPIENT_MASK = 0x1F,
} usb_recipient_t;

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
} usb_request_code_t;

// USB_Features
typedef enum
{
	USB_FEATURE_ENDPOINT_HALT = 0,	
	USB_FEATURE_DEVICE_REMOTE_WAKEUP = 1,
	USB_FEATURE_DEVICE_TEST_MODE = 2,
} usb_feature_t;

typedef struct
{
	unsigned char RequestType;
	unsigned char RequestCode;
	unsigned short Value;
	unsigned short Index;
	unsigned short Length;
} usb_request_t;

#define USB_REQ_INDEX_EP_INPUT 0x80


// descriptors info
/////////////////////////

typedef struct __packed
{
	unsigned char Bytes[2];
} usb16_t;

typedef struct __packed
{
	unsigned char Bytes[4];
} usb32_t;

#define USB16TOH(u) (((u).Bytes[1] << 8) | ((u).Bytes[0]))
#define HTOUSB16(h) (usb16_t){ (unsigned char)((h) & 0xFF), (unsigned char)((h) >> 8) }

#define USB32TOH(u) (((u).Bytes[3] << 24) | ((u).Bytes[2] << 16) | ((u).Bytes[1] << 8) | ((u).Bytes[0]))
#define HTOUSB32(h) (usb32_t){ (unsigned char)((h) & 0xFF), (unsigned char)((h) >> 8), (unsigned char)((h) >> 16), (unsigned char)((h) >> 24) }

typedef struct __packed
{
	unsigned char Length;
	unsigned char DescriptorType;
} usb_descriptor_header_t;

typedef struct __packed
{
	usb_descriptor_header_t Header;
	usb16_t USBVersion;
	unsigned char DeviceClass;
	unsigned char DeviceSubClass;
	unsigned char DeviceProtocol;
	unsigned char MaxPacketSize;	// for endpoint 0
	usb16_t VendorId;
	usb16_t ProductId;
	usb16_t DeviceVersion;
	unsigned char ManufacturerIndex;
	unsigned char ProductIndex;
	unsigned char SerialNumberIndex;
	unsigned char NumConfigurations;
} usb_device_descriptor_t;

typedef struct __packed
{
	usb_descriptor_header_t Header;
	usb16_t TotalLength;
	unsigned char NumInterfaces;
	unsigned char ConfigurationValue;
	unsigned char ConfigurationIndex;
	union __packed
	{
		unsigned char Attributes;
		struct __packed
		{
			unsigned :5;
			unsigned RemoteWakeup:1;
			unsigned SelfPowered:1;
			unsigned Res1_BusPowered:1;
		} AttributesBits;
	};
	unsigned char MaxPower;	// bus-power, in 2mA units
} usb_configuration_descriptor_t;

typedef struct __packed
{
	usb_descriptor_header_t Header;
	unsigned char FirstInterface;
	unsigned char InterfaceCount;
	unsigned char FunctionClass;
	unsigned char FunctionSubclass;
	unsigned char FunctionProtocol;
	unsigned char FunctionNameIndex; 
} usb_interface_association_descriptor_t;

typedef struct __packed
{
	usb_descriptor_header_t Header;
	unsigned char InterfaceNumber;
	unsigned char AlternateSetting;
	unsigned char NumEndpoints;
	unsigned char InterfaceClass;
	unsigned char InterfaceSubClass;
	unsigned char Protocol;
	unsigned char InterfaceNameIndex;	
} usb_interface_descriptor_t;

typedef enum
{
	USB_TT_CONTROL = 0,
	USB_TT_ISO = 1,
	USB_TT_BULK = 2,
	USB_TT_INTERRUPT = 3,
} usb_transfer_type_t;

typedef enum
{
	USB_HOST_TO_DEVICE = 0,
	USB_DEVICE_TO_HOST = 1,
} usb_direction_t;

typedef struct __packed
{
	usb_descriptor_header_t Header;
	union __packed
	{
		unsigned char Address;
		struct __packed
		{
			unsigned EndpointNumber:4;
			unsigned :3;
			unsigned Input:1;
		} AddressBits;
	};
	union __packed
	{
		unsigned char Attributes;
		struct __packed
		{
			unsigned TransferType:2;
			unsigned IsoSyncType:2;
			unsigned IsoUsageType:2;
			unsigned :2;
		} AttributesBits;
	};
	usb16_t MaxPacketSize;
	unsigned char Interval;
} usb_endpoint_descriptor_t;

typedef struct __packed
{
	usb_descriptor_header_t Header;
	usb16_t String[0];
} usb_string_descriptor_t;

typedef struct __packed
{
	usb_descriptor_header_t Header;
	unsigned char FirstInterface;
	unsigned char InterfaceCount;
	unsigned char FunctionClass;
	unsigned char FunctionSubClass;
	unsigned char FunctionProtocol;
	unsigned char FunctionIndex;
} usb_if_association_descriptor_t;

typedef enum 
{
	USB_USB_1_0 = 0x0100,
	USB_USB_1_1 = 0x0110,
	USB_USB_2_0 = 0x0200,
} usb_spec_t;

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
} usb_class_t;

typedef enum
{
	USB_DEVICE_SUBCLASS_COMMON = 2,	// used with USB_CLASS_MISC for Interface Association
} usb_subclass_t;

typedef enum
{
	USB_DEVICE_PROTOCOL_IAD = 1, // Interface Association Descriptor
} usb_protocol_t;

// bmAttributes in Configuration Descriptors
#define USB_CONFIG_POWERED_MASK                0xC0
#define USB_CONFIG_BUS_POWERED                 0x80
#define USB_CONFIG_SELF_POWERED                0x40
#define USB_CONFIG_REMOTE_WAKEUP               0x20

typedef enum
{
	USB_STATUS_SELF_POWERED = 0x01,
	USB_STATUS_REMOTE_WAKEUP  = 0x02,
} usb_status_t;

// prototypes
int usb_parse_conf(usb_configuration_descriptor_t *conf_desc, usb_descriptor_type_t desc_type, usb_descriptor_header_t **current);
int usb_desc2str(usb_descriptor_header_t *str_desc, unsigned char *dest, int max_length);

#endif // USB_H
