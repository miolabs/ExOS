#ifndef APPLE_IAP2_H
#define APPLE_IAP2_H

#include <kernel/types.h>
#include <kernel/iobuffer.h>
#include <stdbool.h>

#ifndef IAP2_MAX_SESSIONS
#define IAP2_MAX_SESSIONS 2
#endif

typedef enum
{
	IAP2_LINGO_GENERAL = 0,
} iap2_lingo_t;

typedef struct
{
	unsigned char High, Low; 
} iap2_short_t;

#define IAP2SHTOH(sh) (((sh).High << 8) | (sh).Low)
#define HTOIAP2S(s) (iap2_short_t){ .High = (s) >> 8, .Low = (s) & 0xff }

#define IAP2_CONTROLF_SYN 0x80
#define IAP2_CONTROLF_ACK 0x40
#define IAP2_CONTROLF_EAK 0x20
#define IAP2_CONTROLF_RST 0x10
#define IAP2_CONTROLF_SLP 0x08

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

typedef enum
{
	IAP2_SESSION_TYPE_CONTROL = 0,
	IAP2_SESSION_TYPE_FILE_TRANSFER = 1,
	IAP2_SESSION_TYPE_EXTERNAL_ACCESSORY = 2,
} iap2_session_type_t;

typedef struct __packed
{
	unsigned char Id;
	unsigned char Type;
	unsigned char Ver;
} iap2_link_session1_t;

typedef struct __packed
{
	unsigned char LinkVersion;	// NOTE: always 1
	unsigned char MaxNumOutstandingPackets;
	iap2_short_t MaxRcvPacketLength;
	iap2_short_t RetxTimeout;
	iap2_short_t CumulativeAckTimeout;
	unsigned char MaxRetx;
	unsigned char MaxCumulativeAcks;
	iap2_link_session1_t Sessions[0];
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
	bool (*SwitchRole)(iap2_transport_t *t);
	// TODO
};

typedef struct
{
	unsigned char MaxNumOutstandingPackets;
	unsigned short MaxRcvPacketLength;
	unsigned short RetxTimeout;
	unsigned short CumulativeAckTimeout;
	unsigned char MaxRetx;
	unsigned char MaxCumulativeAcks;
} iap2_link_params_t;

typedef struct
{
	iap2_transport_t *Transport;
	unsigned char Seq;
	unsigned char Ack;

	iap2_link_params_t LinkParams;
	event_t SyncEvent;

	iap2_link_session1_t Sessions[IAP2_MAX_SESSIONS];
} iap2_context_t;

void iap2_initialize();
bool iap2_transport_create(iap2_transport_t *t, const char *id, const iap2_transport_driver_t *driver);
bool iap2_start(iap2_transport_t *t);
void iap2_input(iap2_transport_t *t, const unsigned char *packet, unsigned packet_length);
void iap2_stop();

#endif // APPLE_IAP2_H
