#ifndef USB_DEVICE_CDC_NCM_H
#define USB_DEVICE_CDC_NCM_H

#include <usb/classes/cdc.h>
#include <usb/configuration.h>
#include <usb/device.h>
#include <kernel/tree.h>
#include <kernel/iobuffer.h>
#include <kernel/io.h>

extern const usb_device_interface_driver_t *__usb_cdc_ncm_device_driver;

// NOTE: cdc notifications mimic an usb_request 8 byte header, followed by actual data
typedef struct
{
	unsigned char RequestType;
	unsigned char RequestCode;
	unsigned short Value;
	unsigned short Index;
	unsigned short Length;
} usb_cdc_notify_header_t;

typedef enum
{
	USB_CDC_NOTIFY_NETWORK_CONNECTION = 0x00,
	USB_CDC_NOTIFY_RESPONSE_AVAILABLE = 0x01,
	// TODO
	USB_CDC_NOTIFY_CONNECTION_SPEED_CHANGE = 0x2a,
} usb_cdc_notify_code_t;

typedef struct
{
	usb32_t USBitRate;
	usb32_t DSBitRate;
} usb_cdc_notify_speed_t;

typedef struct
{
	usb_descriptor_header_t Header;
	unsigned char DescriptorSubtype;	// USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_ENF
	unsigned char iMACAddress;			// index of string descriptor containing the mac address in hex
	usb32_t	bmEthernetStatistics;
	usb16_t wMaxSegmentSize;
	usb16_t wNumberMCFilters;
	unsigned char bNumberPowerFilters;
} usb_cdc_ethernet_functional_descriptor_t;

typedef struct
{
	usb_descriptor_header_t Header;
	unsigned char DescriptorSubtype;	// USB_CDC_FUNC_DESCRIPTOR_SUBTYPE_NCM
	usb16_t bcdNcmVersion;
	unsigned char bmNetworkCapabilities;
} usb_cdc_ncm_functional_descriptor_t;

#define USB_CDC_NCM_NETCAPSF_SetEthernetPacketFilters	(1<<0)
#define USB_CDC_NCM_NETCAPSF_SetGetNetAddress			(1<<1)
#define USB_CDC_NCM_NETCAPSF_SetGetEncapsulatedCmdResp	(1<<2)
#define USB_CDC_NCM_NETCAPSF_SetGetMaxDatagramSize		(1<<3)
#define USB_CDC_NCM_NETCAPSF_SetGetCrcMode				(1<<4)


#define NCMSIG(a, b, c, d) (((a) << 24)|((b) << 16)|((c) << 8)|(d)
#define SIG_NTH16 NCMSIG('N', 'C', 'M', 'H')
#define SIG_NDP16 NCMSIG('N', 'C', 'M', 'x')
#define SIG_NTH32 NCMSIG('n', 'c', 'm', 'h')
#define SIG_NDP32 NCMSIG('n', 'c', 'm', 'x')

// NOTE: each usb transfer is transfer block (ntb) and begins with an ncm transfer header (nth)
// NOTE: nth points to the head of a list of datagram pointers (ndp)
// NOTE: each ndp points to one or more ethernet frames, encapsulated within the ntb
// NOTE: within the ntb, nth must always be the first, but the others may occur in any order

typedef struct
{
	usb32_t	dwSignature;	// NTH16
	usb16_t wHeaderLength;	// header length (=12)
	usb16_t wSequence;		// sequence number reset by NCM reset (select alt-setting 0)
	usb16_t wBlockLength;	// NTB length
	usb16_t wNdpIndex;		// offset of first NDP (4-aligned)
} ncm_transfer_header16_t;	// NTH

typedef struct
{
	usb32_t	dwSignature;	// NTH32
	usb16_t wHeaderLength;	// header length (=16)
	usb16_t wSequence;		// sequence number reset by NCM reset (select alt-setting 0)
	usb32_t dwBlockLength;	// NTB length
	usb32_t dwNdpIndex;		// offset of first NDP (4-aligned)
} ncm_transfer_header32_t;	// NTH

typedef struct
{
	usb32_t dwSignature;	// NDP16
	usb16_t wHeaderLength;	// header length (=8+k*4, k>=2)
	usb16_t wNextNdpIndex;	// offset of next DP (or zero, for last one)
	struct ncm_datagram16
	{
		usb16_t wDatagramIndex;		// offset of datagram
		usb16_t wDatagramLength;	// datagram length, 0 = end
	} array[0];
} ncm_datagram_pointer16_t;

typedef struct
{
	usb32_t dwSignature;	// NDP32
	usb16_t wHeaderLength;	// header length (=16+k*8, k>=2)
	usb16_t wPad;
	usb32_t dwNextNdpIndex;	// offset of next DP (or zero, for last one)
	usb32_t dwPad;
	struct ncm_datagram32
	{
		usb32_t wDatagramIndex;		// offset of datagram
		usb32_t wDatagramLength;	// datagram length, 0 = end
	} array[0];
} ncm_datagram_pointer32_t;

typedef enum
{
	NCM_GET_NTB_PARAMETERS = 0x80,
	NCM_GET_NET_ADDRESS,
	NCM_SET_NET_ADDRESS,
	NCM_GET_NTB_FORMAT,
	NCM_SET_NTB_FORMAT,
	NCM_GET_NTB_INPUT_SIZE,
	NCM_SET_NTB_INPUT_SIZE,
	NCM_GET_NTB_MAX_DATAGRAM_SIZE,
	NCM_SET_NTB_MAX_DATAGRAM_SIZE,
	NCM_GET_CRC_MODE,
	NCM_SET_CRC_MODE,
} ncm_req_code_t;

typedef struct
{
	usb16_t wLength;
	usb16_t bmNtbFormatsSupported;
	usb32_t dwNtbInMaxSize;
	usb16_t wNdpInDivisor;
	usb16_t wNdpInPayloadRemainder;
	usb16_t wNdpInAlignment;
	usb16_t wPad14;
	usb32_t dwNtbOutMaxSize;
	usb16_t wNdpOutDivisor;
	usb16_t wNdpOutPayloadRemainder;
	usb16_t wNdpOutAlignment;
	usb16_t wNtbOutMaxDatagrams;
} ncm_ntb_parameter_structure_t;

#define NCM_NTB_FORMAT_SUPP_NTB16	(1<<0)
#define NCM_NTB_FORMAT_SUPP_NTB32	(1<<1)

typedef struct
{
	bool Connected;
	unsigned short SpeedMbps;
} ncm_notify_state_t;


#define NCM_MAX_PACKET_LENGTH 64

#define NCM_OUTPUT_EP_BUFFER	4096
#define NCM_INPUT_EP_BUFFER		4096
#define NCM_NOTIFY_EP_BUFFER	NCM_MAX_PACKET_LENGTH

typedef struct
{
	node_t Node;
	usb_device_interface_t *Interface;
	usb_device_interface_t SecondaryDataInterface;
	mutex_t Lock;

	unsigned char Unit;
	bool Ready;
	bool Idle;
	bool Connected;
	unsigned short SpeedMbps;

	char EthernetMacString[16];
	usb_device_string_t EthernetMac;	// NOTE: for ECM func decsriptor (iMACAddress)
	dispatcher_context_t *DispatcherContext;

	ncm_notify_state_t NotifyState;
	usb_io_buffer_t NotifyIo;
	event_t NotifyEvent;
	dispatcher_t NotifyDispatcher;

	//io_buffer_t Output;
	//io_buffer_t Input;
	
	usb_io_buffer_t TxIo;
	event_t TxEvent;
	dispatcher_t TxDispatcher;
	
	usb_io_buffer_t RxIo;
	event_t RxEvent;
	dispatcher_t RxDispatcher;
	
	unsigned char TxData[NCM_INPUT_EP_BUFFER];
	unsigned char RxData[NCM_OUTPUT_EP_BUFFER];

	unsigned char NotifyData[NCM_NOTIFY_EP_BUFFER];

	//char DeviceName[16];
} ncm_device_context_t;


#endif // USB_DEVICE_CDC_NCM_H


