#ifndef USB_DRIVER_HID_H
#define USB_DRIVER_HID_H

#include <usb/classes/hid.h>
#include <usb/host.h>
#include <kernel/event.h>

typedef struct
{
	USB_HOST_FUNCTION;
	USB_HOST_PIPE InputPipe;
   	unsigned char Interface;
	unsigned char MaxReportId;

	EXOS_MUTEX InputLock;
	EXOS_LIST Inputs;

	unsigned char InputBuffer[64];
	unsigned char OutputBuffer[64];
} HID_FUNCTION;

typedef struct _HID_REPORT_HANDLER HID_REPORT_HANDLER;

typedef struct
{
	EXOS_NODE Node;
	HID_REPORT_HANDLER *Handler;
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
	int (*MatchDevice)(USB_HOST_DEVICE *device);
	HID_REPORT_HANDLER *(*MatchInputHandler)(HID_FUNCTION *func, HID_REPORT_INPUT *input);
	void (*EndReportEnum)(HID_FUNCTION *func);
} HID_REPORT_DRIVER;

typedef struct
{
	EXOS_NODE Node;
	const HID_REPORT_DRIVER *Driver;
} HID_REPORT_MANAGER;


struct _HID_REPORT_HANDLER
{
	EXOS_EVENT ReceiveEvent;
	unsigned char *Data;
	HID_FUNCTION *Function;
	HID_REPORT_INPUT *Input;
};

#define HID_REQUEST_SET_REPORT 0x09

void usbd_hid_initialize();
int usbd_hid_add_manager(HID_REPORT_MANAGER *manager); 
int usbd_hid_set_report(HID_FUNCTION *func, unsigned char report_type, unsigned char report_id, void *data, int length);

#endif // USB_DRIVER_HID_H

