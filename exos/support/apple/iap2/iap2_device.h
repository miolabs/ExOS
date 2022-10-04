#ifndef APPLE_IAP2_DEVICE_H
#define APPLE_IAP2_DEVICE_H

#include <usb/configuration.h>
#include <usb/device.h>
#include <kernel/tree.h>
#include <kernel/iobuffer.h>
#include <kernel/io.h>

//#ifndef IAP2_MAX_INTERFACES 
//#define IAP2_MAX_INTERFACES 1
//#endif

extern const usb_device_interface_driver_t *__usb_iap2_device_driver;

typedef enum
{
	IAP2_VENDOR_RESET = 0x0,
	// TODO
} iap2_vendor_request_t;


#define IAP2_MAX_PACKET_LENGTH 64

#ifndef IAP2_INPUT_BUFFER
#define IAP2_INPUT_BUFFER 256
#endif

#ifndef IAP2_OUTPUT_BUFFER
#define IAP2_OUTPUT_BUFFER 256
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
	
	unsigned char TxData[IAP2_MAX_PACKET_LENGTH];
	unsigned char RxData[IAP2_MAX_PACKET_LENGTH];
	unsigned char TxSize;
	unsigned char Latency;
	bool Ready;
	bool Idle;

	unsigned char OutputBuffer[IAP2_OUTPUT_BUFFER];
	unsigned char InputBuffer[IAP2_INPUT_BUFFER];

	char DeviceName[16];
} iap2_device_context_t;

// prototypes
bool iap2_vendor_request(usb_request_t *req, void **pdata, int *plength);

#endif // APPLE_IAP2_DEVICE_H


