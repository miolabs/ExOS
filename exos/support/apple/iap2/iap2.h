#ifndef APPLE_IAP2_H
#define APPLE_IAP2_H

#include <stdbool.h>

#define USB_IAP_REQ_DEVICE_POWER_REQUEST 0x40

typedef struct
{
	// TODO
} iap2_header_t;


typedef struct iap2_transport_driver iap2_transport_driver_t;

typedef struct 
{
	const char *Id;
	const iap2_transport_driver_t *Driver;
	unsigned Transaction;
} iap2_transport_t;

struct iap2_transport_driver
{
	bool (*Send)(iap2_transport_t *t, const unsigned char *data, unsigned length);
	// TODO
};

void iap2_initialize();
bool iap2_transport_create(iap2_transport_t *t, const char *id, const iap2_transport_driver_t *driver);
bool iap2_start(iap2_transport_t *t);
void iap2_stop();

#endif // APPLE_IAP2_H
