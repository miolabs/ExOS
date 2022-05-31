#ifndef USB_DRIVER_HID_H
#define USB_DRIVER_HID_H

#include <usb/classes/hid.h>
#include <usb/host.h>
#include <kernel/dispatch.h>

typedef struct __HID_FUNCTION_HANDLER HID_FUNCTION_HANDLER;

typedef struct
{
	USB_HOST_FUNCTION;
	USB_HOST_PIPE InputPipe;
	unsigned char InterfaceSubClass;
	unsigned char Protocol;
   	unsigned char Interface;
	unsigned char MaxReportId;

	HID_FUNCTION_HANDLER *Handler;
	EXOS_MUTEX InputLock;
	EXOS_LIST Inputs;

	unsigned char InputBuffer[64];
	unsigned char OutputBuffer[64];
	
	unsigned char ExitFlag;
	unsigned char StartedFlag;
	unsigned char InstanceIndex;

	USB_REQUEST_BUFFER Urb;
	EXOS_DISPATCHER Dispatcher;
} HID_FUNCTION;

typedef struct
{
	EXOS_NODE Node;
	//HID_FUNCTION_HANDLER *Handler;
	unsigned char ReportId;
	unsigned char Offset;
	unsigned char UsagePage;
	unsigned char Usage;
	unsigned char Size;
	unsigned char Count;
	unsigned char Min;
	unsigned char Max;
	unsigned long InputFlags;
} HID_REPORT_INPUT;

typedef struct 
{
	HID_FUNCTION_HANDLER *(*MatchDevice)(HID_FUNCTION *func);
	int (*MatchInputHandler)(HID_FUNCTION_HANDLER *handler, HID_REPORT_INPUT *input);
	void (*Start)(HID_FUNCTION_HANDLER *handler);
	void (*Stop)(HID_FUNCTION_HANDLER *handler);
	void (*Notify)(HID_FUNCTION_HANDLER *handler, HID_REPORT_INPUT *input, unsigned char *data);
} HID_DRIVER;

typedef struct
{
	EXOS_NODE Node;
	const HID_DRIVER *Driver;
} HID_DRIVER_NODE;

struct __HID_FUNCTION_HANDLER
{
	EXOS_NODE Node;
	HID_DRIVER_NODE *DriverNode;
	HID_FUNCTION *Function;
};

//struct _HID_REPORT_HANDLER
//{
//	EXOS_NODE Node;
//	
//	HID_FUNCTION *Function;
//	HID_REPORT_INPUT *Input;
//};

#define HID_REQUEST_SET_REPORT 0x09

void usbd_hid_initialize();
int usbd_hid_add_driver(HID_DRIVER_NODE *node); 
int usbd_hid_set_report(HID_FUNCTION *func, unsigned char report_type, unsigned char report_id, void *data, int length);

#endif // USB_DRIVER_HID_H

