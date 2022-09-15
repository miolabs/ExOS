#ifndef APPLE_IAP2_HIDD_H
#define APPLE_IAP2_HIDD_H

#include <support/usb/driver/hid.h>
#include "iap2.h"

#define IAP2_MAX_REPORTS 5
#define IAP2_MAX_INPUT_BUFFER 512

typedef struct
{
	hid_function_handler_t Hid;
	struct iap2_hid_report { unsigned char ReportId; unsigned char Length; } Reports[IAP2_MAX_REPORTS];
	unsigned ReportsCount;
	iap2_transport_t Transport;
	unsigned short InputOffset;
	unsigned char InputBuffer[IAP2_MAX_INPUT_BUFFER];
} iap2_hid_handler_t;

// link control byte flags
#define IAP2_LCB_START (0)
#define IAP2_LCB_CONTINUATION (1<<0)
#define IAP2_LCB_MORE_TO_FOLLOW (1<<1)


// prototypes
void iap2_hidd_initialize();

#endif // APPLE_IAP2_HIDD_H


