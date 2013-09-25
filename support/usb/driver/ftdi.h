#ifndef EXOS_USB_FTDI_DRIVER_H
#define EXOS_USB_FTDI_DRIVER_H

#include <usb/host.h>
#include <kernel/tree.h>
#include <kernel/dispatch.h>
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

typedef struct
{
	USB_HOST_FUNCTION;

	USB_HOST_PIPE BulkInputPipe;
	USB_HOST_PIPE BulkOutputPipe;
	unsigned char OutputBuffer[FTDI_USB_BUFFER];	// used for output and setup
	unsigned char InputBuffer[FTDI_USB_BUFFER];

	unsigned char Interface;
	unsigned char Protocol;
	unsigned char DeviceUnit;
	unsigned char Reserved;
} FTDI_FUNCTION;

typedef enum
{
	FTDI_HANDLE_NOT_MOUNTED = 0,
	FTDI_HANDLE_CLOSED,
	FTDI_HANDLE_OPENING,
	FTDI_HANDLE_READY,
	FTDI_HANDLE_ERROR,
} FTDI_HANDLE_STATE;

typedef struct
{
	COMM_IO_ENTRY *Entry;
	EXOS_DISPATCHER Dispatcher;
	EXOS_TREE_DEVICE KernelDevice;
	const char *DeviceName;
	FTDI_FUNCTION *Function;

	FTDI_HANDLE_STATE State;
   	USB_REQUEST_BUFFER Request;
	EXOS_IO_BUFFER IOBuffer;
   	unsigned char Buffer[FTDI_IO_BUFFER];	
} FTDI_HANDLE;

typedef struct
{
	FTDI_HANDLE *Handle;
	EXOS_EVENT DoneEvent;
} FTDI_HANDLE_REQUEST;

// prototypes
void ftdi_initialize();

#endif // EXOS_USB_FTDI_DRIVER_H


