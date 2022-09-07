#ifndef USB_DEVICE_FTDI_H
#define USB_DEVICE_FTDI_H

#include <usb/configuration.h>
#include <usb/device.h>
#include <kernel/tree.h>
#include <kernel/iobuffer.h>
#include <kernel/io.h>

#ifndef FTDI_MAX_INTERFACES 
#define FTDI_MAX_INTERFACES 1
#endif

extern const usb_device_interface_driver_t *__usb_ftdi_driver;

typedef enum
{
	FTDI_VENDOR_RESET = 0x0,
	FTDI_VENDOR_MODEM_CTRL = 0x01,
	FTDI_VENDOR_FLOW_CTRL = 0x02,
	FTDI_VENDOR_SET_BAUDRATE = 0x03,
	FTDI_VENDOR_SET_FRAME = 0x04,	// AKA SET_DATA
	FTDI_VENDOR_MODEM_POLL = 0x05,
	FTDI_VENDOR_FLUSH_DATA = 0x06,
	FTDI_VENDOR_SET_LATENCY = 0x09,
	FTDI_VENDOR_GET_LATENCY = 0x0A,
	FTDI_VENDOR_READ_EEPROM = 0x90,
	FTDI_VENDOR_WRITE_EEPROM = 0x91,
	FTDI_VENDOR_ERASE_EEPROM = 0x92,
} ftdi_vendor_request_t;

#define FTDI_VENDOR_SET_FRAME_PARITY_BIT 8
#define FTDI_VENDOR_SET_FRAME_PARITY_NONE (0x0 << FTDI_VENDOR_SET_FRAME_PARITY_BIT)
#define FTDI_VENDOR_SET_FRAME_PARITY_ODD (0x1 << FTDI_VENDOR_SET_FRAME_PARITY_BIT)
#define FTDI_VENDOR_SET_FRAME_PARITY_EVEN (0x2 << FTDI_VENDOR_SET_FRAME_PARITY_BIT)
#define FTDI_VENDOR_SET_FRAME_PARITY_MARK (0x3 << FTDI_VENDOR_SET_FRAME_PARITY_BIT)
#define FTDI_VENDOR_SET_FRAME_PARITY_SPACE (0x4 << FTDI_VENDOR_SET_FRAME_PARITY_BIT)
#define FTDI_VENDOR_SET_FRAME_STOP_BIT 11
#define FTDI_VENDOR_SET_FRAME_STOP_BITS_1 (0x0 << FTDI_VENDOR_SET_FRAME_STOP_BIT)
#define FTDI_VENDOR_SET_FRAME_STOP_BITS_15 (0x1 << FTDI_VENDOR_SET_FRAME_STOP_BIT)
#define FTDI_VENDOR_SET_FRAME_STOP_BITS_2 (0x2 << FTDI_VENDOR_SET_FRAME_STOP_BIT)
#define FTDI_VENDOR_SET_FRAME_BREAK (0x1 << 14)

#define FTDI_VENDOR_MODEM_CTRL_DTR (1<<0)
#define FTDI_VENDOR_MODEM_CTRL_RTS (1<<1)

typedef struct
{
	unsigned char HighCurrent;	// type R
	unsigned char EndpointSize;
	usb16_t VendorID;
	usb16_t ProductID;
	usb16_t ProductVersion;

	unsigned char UsbFeatures;
	unsigned char UsbMaxPower;
	unsigned char SuspendPulldown;	// type R
	unsigned char Invert;			// type R
	usb16_t UsbVersion;
	unsigned char ManufacturerOffset;
	unsigned char ManufacturerLength;
	
	unsigned char ProductOffset;
	unsigned char ProductLength;
	unsigned char SerialOffset;
	unsigned char SerialLength;
	unsigned char CBusFunc[4];
} ftdi_eeprom_t;

#define FTDI_MAX_PACKET_LENGTH 64

#ifndef FTDI_INPUT_BUFFER
#define FTDI_INPUT_BUFFER 256
#endif

#ifndef FTDI_OUTPUT_BUFFER
#define FTDI_OUTPUT_BUFFER 256
#endif

typedef struct
{
	node_t Node;
	unsigned char Unit;
//	unsigned Timeout;
	io_tree_device_t DeviceNode;
	mutex_t Lock;
	io_entry_t *Entry;

	io_buffer_t Output;
	io_buffer_t Input;

	usb_device_interface_t *Interface;
	dispatcher_context_t *DispatcherContext;
	
	usb_io_buffer_t TxIo;
	event_t TxEvent;
	dispatcher_t TxDispatcher;
	
	usb_io_buffer_t RxIo;
	event_t RxEvent;
	dispatcher_t RxDispatcher;
	
	unsigned char TxData[FTDI_MAX_PACKET_LENGTH];
	unsigned char RxData[FTDI_MAX_PACKET_LENGTH];
	unsigned char TxSize;
	unsigned char Latency;
	bool Ready;
	bool Idle;

	unsigned char OutputBuffer[FTDI_OUTPUT_BUFFER];
	unsigned char InputBuffer[FTDI_INPUT_BUFFER];

	char DeviceName[16];
} ftdi_context_t;

// prototypes
bool ftdi_vendor_request(usb_request_t *req, void **pdata, int *plength);
void ftdi_hook_dtr(ftdi_context_t *ftdi, bool dtr) __weak;

#endif // USB_DEVICE_FTDI_H


