#ifndef USB_CLASS_CDC_H
#define USB_CLASS_CDC_H

#include "usb/usb.h"

typedef enum
{
	USB_CDC_SUBCLASS_RESERVED = 0,
	USB_CDC_SUBCLASS_DLCM = 1,	// Direct Line Control Model
	USB_CDC_SUBCLASS_ACM = 2,	// Abstract Control Model
	USB_CDC_SUBCLASS_TCM = 3,	// Telephony Control Model
	USB_CDC_SUBCLASS_MCCM = 4,	// Multi-Channel Control Model
	USB_CDC_SUBCLASS_CAPI = 5,	// CAPI Control Model
	USB_CDC_SUBCLASS_ENCM = 6,	// Ethernet Networking Model
	USB_CDC_SUBCLASS_ATM = 7,	// ATM Networking Control Model
} USB_CDC_SUBCLASS;

typedef enum
{
	USB_CDC_PROTOCOL_NONE = 0,
	USB_CDC_PROTOCOL_V250 = 1,
	USB_CDC_PROTOCOL_PCCA101 = 2,
	USB_CDC_PROTOCOL_PCCA101_AO = 3,
	USB_CDC_PROTOCOL_GSM = 4,
	USB_CDC_PROTOCOL_3GPP = 5,
	USB_CDC_PROTOCOL_CDMA = 6,
	USB_CDC_PROTOCOL_EEM = 7,
	USB_CDC_PROTOCOL_EXTERNAL = 0xFE,
	USB_CDC_PROTOCOL_VENDOR_SPECIFIC = 0xFF,
} USB_CDC_PROTOCOL;

typedef enum
{
	USB_DATA_PROTOCOL_NONE = 0,
	USB_DATA_PROTOCOL_EXTERNAL = 0xFE,
	USB_DATA_PROTOCOL_VENDOR_SPECIFIC = 0xFF,
} USB_DATA_PROTOCOL;

typedef enum
{
	USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_HEADER = 0x00,
	USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_CALL_MANAGEMENT = 0x01,
    USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_ACM = 0x02,
    USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_DLM = 0x03,
    USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_RINGER = 0x04,
    USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_TCLSRC = 0x05,
    USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_UFD = 0x06,	// Union Functional Descriptor
    USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_COUNTRY_SELECTION = 0x07,
    USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_TOM = 0x08,
    USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_USBTF = 0x09,
    USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_NCT = 0x0A,
    USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_PUFD = 0x0B,	// Protocol Unit Functional Descriptor
    USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_EUFD = 0x0C,	// Extension Unit Functional Descriptor
    USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_MCMFD = 0x0D,
    USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_CAPI = 0x0E,
    USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_ENFD = 0x0F,	// Ethernet Networking Functional Descriptor
    USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_ATM = 0x10,
    USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_WHCM = 0x11,
    USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_MDLM = 0x12,
	USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_MDLMD = 0x13,
    USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_DMM = 0x14,
    USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_OBEX = 0x15,
    USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_CS = 0x16,
    USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_CSD = 0x17,
    USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_TCM = 0x18,
    USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_OBEX_SI = 0x19,
} USB_CDC_FUNC_DESCRIPTOR_SUBTYPE;

typedef struct
{
	USB_DESCRIPTOR_HEADER Header;
	unsigned char DescriptorSubtype; // USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_HEADER
	USB16 CDC;	// CDC Spec. release number in bcd
} USB_CDC_FUNC_HEADER_DESCRIPTOR;

typedef struct
{
	USB_DESCRIPTOR_HEADER Header;
	unsigned char DescriptorSubtype; // USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_UFD
	unsigned char ControlInterface;
	unsigned char SubordinateInterface[0];
} USB_CDC_UNION_FUNCTIONAL_DESCRIPTOR;

typedef enum
{
	USB_CDC_REQUEST_SEND_ENCAPSULATED_COMMAND = 0x00,
    USB_CDC_REQUEST_GET_ENCAPSULATED_RESPONSE = 0x01,
} USB_CDC_REQUEST_CODE;

#endif // USB_CLASS_CDC_H