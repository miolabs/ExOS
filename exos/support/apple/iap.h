#ifndef APPLE_IAP_H
#define APPLE_IAP_H

#include <support/usb/driver/hid.h>

#define IAP_MAX_REPORT_INPUTS 5
#define IAP_MAX_INPUT_BUFFER 512

typedef struct
{
	hid_function_handler_t Hid;
	struct iap_input_report { unsigned char ReportId; unsigned char Length; } Inputs[IAP_MAX_REPORT_INPUTS];
	unsigned char InputsCount;
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


