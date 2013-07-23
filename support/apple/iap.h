#ifndef APPLE_IAP_H
#define APPLE_IAP_H

#include <support/usb/driver/hid.h>

// link control byte flags
#define IAP_LCB_START (0)
#define IAP_LCB_CONTINUATION (1<<0)
#define IAP_LCB_MORE_TO_FOLLOW (1<<1)

#define USB_IAP_REQ_DEVICE_POWER_REQUEST 0x40

void iap_initialize();

#endif // APPLE_IAP_H


