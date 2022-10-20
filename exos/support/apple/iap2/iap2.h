#ifndef APPLE_IAP2_H
#define APPLE_IAP2_H

#include <kernel/types.h>
#include <kernel/iobuffer.h>
#include <kernel/fifo.h>
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
	iap2_short_t RetransmitTimeout;
	iap2_short_t CumulativeAckTimeout;
	unsigned char MaxRetransmits;
	unsigned char MaxCumulativeAcks;
	iap2_link_session1_t Sessions[0];
} iap2_link_sync_payload1_t;


typedef struct __packed
{
	iap2_short_t Sop;
	iap2_short_t MessageLength;
	iap2_short_t MessageId;
	struct iap2_control_session_message_parameter
	{
		iap2_short_t Length;
		iap2_short_t Id;
		unsigned char Data[0];
	} Parameter[0];
} iap2_control_sess_message_t;

#define IAP2_CTRL_SOF 0x4040

typedef struct iap2_control_session_message_parameter iap2_control_session_message_parameter_t;

typedef enum
{
	// Accessory Authentication
	IAP2_CTRL_MSGID_RequestAuthenticationCertificate = 0xAA00,	// device
	IAP2_CTRL_MSGID_AuthenticationCertificate = 0xAA01,			// accessory
	IAP2_CTRL_MSGID_RequestAuthenticationChallengeResponse = 0xAA02,	// device
	IAP2_CTRL_MSGID_AuthenticationResponse = 0xAA03,					// accessory
	IAP2_CTRL_MSGID_AuthenticationFailed = 0xAA04,					// device
	IAP2_CTRL_MSGID_AuthenticationSucceeded = 0xAA05,				// device
	IAP2_CTRL_MSGID_AccessoryAuthenticationSerialNumber = 0xAA06,	// accessory
	// Accessory Identification
	IAP2_CTRL_MSGID_StartIdentification = 0x1D00,		// device
	IAP2_CTRL_MSGID_IdentificationInformation = 0x1D01,	// accessory
	IAP2_CTRL_MSGID_IdentificationAccepted = 0x1D02,	// device
	IAP2_CTRL_MSGID_IdentificationRejected = 0x1D03,	// device
	IAP2_CTRL_MSGID_CancelIdentification = 0x1D05,				// accessory
	IAP2_CTRL_MSGID_IdentificationInformationUpdate = 0x1D06,	// accessory
} iap2_ctrl_msgid_t;



// transport definitions

typedef struct iap2_transport_driver iap2_transport_driver_t;

typedef struct
{
	unsigned short MaxRcvPacketLength;
	unsigned short RetransmitTimeout;
	unsigned short CumulativeAckTimeout;
	unsigned char MaxNumOutstandingPackets;
	unsigned char MaxRetransmits;
	unsigned char MaxCumulativeAcks;
} iap2_link_params_t;

typedef struct 
{
	const char *Id;
	const iap2_transport_driver_t *Driver;
	unsigned char Unit;
	unsigned char Transaction;
	iap2_link_params_t LinkParams;
} iap2_transport_t;

struct iap2_transport_driver
{
	bool (*Send)(iap2_transport_t *t, const unsigned char *data, unsigned length);
	bool (*SwitchRole)(iap2_transport_t *t);
};

typedef struct
{
	iap2_transport_t *Transport;
	unsigned char Seq;
	unsigned char Ack;

	event_t SyncEvent;
	fifo_t SyncFifo;

	// NOTE: first session in this array should be a control session i. e. Type == IAP2_SESSION_TYPE_CONTROL (0)
	iap2_link_session1_t Sessions[IAP2_MAX_SESSIONS];
} iap2_context_t;

void iap2_initialize();
bool iap2_transport_create(iap2_transport_t *t, const char *id, unsigned char unit, const iap2_transport_driver_t *driver);
bool iap2_start(iap2_transport_t *t);
void iap2_input(iap2_transport_t *t, const unsigned char *packet, unsigned packet_length);
void iap2_stop();

#endif // APPLE_IAP2_H
