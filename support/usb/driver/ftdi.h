#ifndef EXOS_USB_FTDI_DRIVER_H
#define EXOS_USB_FTDI_DRIVER_H

#include <usb/host.h>
#include <kernel/tree.h>
#include <comm/comm.h>

#define FTDI_USB_BUFFER 64
#define FTDI_IO_BUFFER 256

typedef enum
{
	FTDI_MODE_UART = 0,
	FTDI_MODE_UART_FLOW_CTS_RTS,
	FTDI_MODE_BITBANG,
} FTDI_MODE;

typedef enum
{
	FTDI_REQUEST_RESET = 0x00,
	FTDI_REQUEST_MODEM_CONTROL,
	FTDI_REQUEST_FLOW_CONTROL,
	FTDI_REQUEST_SET_BAUDRATE,
	FTDI_REQUEST_SET_DATA,
	FTDI_REQUEST_GET_MODEM_STATUS,
	FTDI_REQUEST_SET_LATENCY_TIMER = 0x09,
	FTDI_REQUEST_GET_LATENCY_TIMER,
} FTDI_REQUEST;

typedef enum
{
	FTDI_HANDLE_CLOSED = 0,
	FTDI_HANDLE_OPENING,
	FTDI_HANDLE_READY,
	FTDI_HANDLE_CLOSING,
	FTDI_HANDLE_ERROR,
} FTDI_HANDLE_STATE;

typedef struct
{
	EXOS_NODE Node;
	COMM_IO_ENTRY *Entry;
	EXOS_MUTEX Lock;
	FTDI_HANDLE_STATE State;
   	USB_REQUEST_BUFFER Request;
   	unsigned char Buffer[FTDI_IO_BUFFER];	
	EXOS_IO_BUFFER IOBuffer;
} FTDI_HANDLE;

typedef struct
{
	USB_HOST_FUNCTION;

	const char *DeviceName;

	USB_HOST_PIPE BulkInputPipe;
	USB_HOST_PIPE BulkOutputPipe;
	EXOS_TREE_DEVICE KernelDevice;
	unsigned char OutputBuffer[FTDI_USB_BUFFER];	// used for output and setup
	unsigned char InputBuffer[FTDI_USB_BUFFER];

	FTDI_HANDLE AsyncHandle; // TODO: move out to conventional memory
	
	unsigned char Interface;
	unsigned char Protocol;
	unsigned char DeviceUnit;
} FTDI_FUNCTION;

// prototypes
void ftdi_initialize();

#endif // EXOS_USB_FTDI_DRIVER_H


