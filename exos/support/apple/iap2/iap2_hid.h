#ifndef APPLE_IAP2_HID_H
#define APPLE_IAP2_HID_H

#include <support/usb/driver/hid.h>
#include "iap2.h"

#define USB_IAP2_REQ_POWER_CAPABILITY	0x40
#define USB_IAP2_REQ_DEVICE_ROLE_SWITCH	0x51

#define IAP2_MAX_REPORTS 5

typedef struct
{
	hid_function_handler_t Hid;
	struct iap2_hid_report { unsigned char ReportId; unsigned char Length; } Reports[IAP2_MAX_REPORTS];
	unsigned ReportsCount;
	iap2_transport_t Transport;
	unsigned short InputOffset;
	unsigned char InputBuffer[IAP2_BUFFER_SIZE];
} iap2_hid_handler_t;

// link control byte flags
#define IAP2_LCB_START (0)
#define IAP2_LCB_CONTINUATION (1<<0)
#define IAP2_LCB_MORE_TO_FOLLOW (1<<1)


// prototypes
void iap2_hid_initialize();

// overridables
extern bool __iap2_hid_should_switch_role(iap2_hid_handler_t *iap2);


#endif // APPLE_IAP2_HID_H


