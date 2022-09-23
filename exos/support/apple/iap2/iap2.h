#ifndef APPLE_IAP2_H
#define APPLE_IAP2_H

#include <kernel/types.h>
#include <kernel/iobuffer.h>
#include <stdbool.h>

#define USB_IAP2_REQ_DEVICE_POWER_REQUEST 0x40

typedef enum
{
	IAP2_LINGO_GENERAL = 0,
} iap2_lingo_t;

typedef struct
{
	unsigned char High, Low; 
} iap2_short_t;

#define IAP2SHTOH(sh) (((sh).High << 8) | (sh).Low)

typedef struct __packed
{
	iap2_short_t Sop;
	iap2_short_t PacketLength;
	unsigned char Control;
	unsigned char SequenceNumber;
	unsigned char AckNumber;
	unsigned char SessionId;
	unsigned char Checksum;
} iap2_header_t;

typedef struct __packed
{
	unsigned char Identifier;
	unsigned char Type;
	unsigned char Version;
} iap2_link_session_payload1_t;

typedef struct __packed
{
	unsigned char LinkVersion;	// NOTE: always 1
	unsigned char MaxNumOutstandingPackets;
	iap2_short_t MaxRcvPacketLength;
	iap2_short_t RetxTimoout;
	iap2_short_t CumulativeAckTimoout;
	unsigned char MaxRetx;
	unsigned char MaxCumulativeAcks;
	iap2_link_session_payload1_t Sessions[0];
} iap2_link_sync_payload1_t;


// transport definitions

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
void iap2_input(iap2_transport_t *t, const unsigned char *packet, unsigned packet_length);
void iap2_stop();

#endif // APPLE_IAP2_H
