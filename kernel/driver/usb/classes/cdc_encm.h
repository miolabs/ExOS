#ifndef USB_CLASS_CDC_ENCM_H
#define USB_CLASS_CDC_ENCM_H

#include "usb/usb.h"
#include "usb/classes/cdc.h"

// Communication class protocol code
#define USB_CDC_ENCM_PROTOCOL_NONE	0

typedef struct
{
	USB_DESCRIPTOR_HEADER Header;
	unsigned char DescriptorSubtype;	// Ethernet Networking functional descriptor as defined in cdc.h
	unsigned char MACAddress;
	USB32 EthernetStatics;
	USB16 MaxSegmentSize;
	USB16 NumberMCFilter;
	unsigned char NumberPowerFilters;
} USB_CDC_ENCM_FUNCTIONAL_DESCRIPTOR;

typedef enum
{
	USB_CDC_ENCM_REQUEST_SET_ETHERNET_MULTICAST_FILTERS = 0x40,
	USB_CDC_ENCM_REQUEST_SET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER = 0x41,
	USB_CDC_ENCM_REQUEST_GET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER = 0x42,
	USB_CDC_ENCM_REQUEST_SET_ETHERNET_PACKET_FILTER = 0x43,
	USB_CDC_ENCM_REQUEST_GET_ETHERNET_STATISTIC = 0x44,
} USB_CDC_ENCM_REQUEST_CODE;

typedef enum
{
	USB_CDC_ENCM_NOTIFY_NETWORK_CONNECTION = 0x00,
	USB_CDC_ENCM_NOTIFY_RSPONSE_AVAILABLE = 0x01,
	USB_CDC_ENCM_NOTIFY_CONNECTION_SPEED_CHANGE = 0x2A,
} USB_CDC_ENCM_NOTIFICATION;

#endif // USB_CLASS_CDC_ENCM_H
