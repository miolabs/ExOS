#ifndef USB_CLASS_USBPRINT_H
#define USB_CLASS_USBPRINT_H

#include <usb/usb.h>

typedef enum
{
	USBPRINT_SUBCLASS_PRINTER = 1,
} usbprint_subclass_t;

typedef enum
{
	USBPRINT_PROTOCOL_UNKNOWN = 0,
	USBPRINT_PROTOCOL_UNIDIRECTIONAL,
	USBPRINT_PROTOCOL_BIDIRECTIONAL,
	USBPRINT_PROTOCOL_1284_4,
} usbprint_protocol_t;

typedef enum
{
	USBPRINT_REQ_GET_DEVICE_ID = 0,
	USBPRINT_REQ_GET_PORT_STATUS,
	USBPRINT_REQ_SOFT_RESET,
} usbprint_request_code_t;

typedef enum
{
	USBPRINT_PORT_NO_ERROR = (1<<3),
	USBPRINT_PORT_SELECT = (1<<4),
	USBPRINT_PORT_PAPER_EMPTY = (1<<5),
} usbprint_port_status_t;

#endif // USB_CLASS_USBPRINT_H


