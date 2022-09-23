#ifndef APPLE_IAP_H
#define APPLE_IAP_H

#include <support/usb/driver/hid.h>

#define IAP_MAX_REPORT_OUTPUTS 5
#define IAP_MAX_INPUT_BUFFER 512

typedef struct __packed
{
	unsigned char ReportId; 
	unsigned char Length;
} iap_output_report_t;

typedef struct
{
	hid_function_handler_t Hid;
	iap_output_report_t OutputReports[IAP_MAX_REPORT_OUTPUTS];
	unsigned char OutputReportCount;
	unsigned long Offset;
	unsigned char Buffer[IAP_MAX_INPUT_BUFFER];
} iap_hid_handler_t;

// link control byte flags
#define IAP_LCB_START (0)
#define IAP_LCB_CONTINUATION (1<<0)
#define IAP_LCB_MORE_TO_FOLLOW (1<<1)

#define USB_IAP_REQ_DEVICE_POWER_REQUEST 0x40

void iap_initialize();

#endif // APPLE_IAP_H


