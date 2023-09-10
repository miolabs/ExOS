#ifndef APPLE_IAP2_H
#define APPLE_IAP2_H

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/iobuffer.h>
#include <kernel/dispatch.h>
#include <kernel/fifo.h>
#include <stdbool.h>

#ifndef IAP2_MAX_SESSIONS
#define IAP2_MAX_SESSIONS 2
#endif

#ifndef IAP2_BUFFER_SIZE
#define IAP2_BUFFER_SIZE 4096
#endif

typedef enum
{
	IAP2_LINGO_GENERAL = 0,
} iap2_lingo_t;

typedef struct __packed
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

typedef struct
{
	unsigned short MaxLength; 
	unsigned short Length;
	void *Buffer;
} iap2_control_parameters_t;

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
	// External Accesory
	IAP2_CTRL_MSGID_StartExternalAccessoryProtocolSession = 0xEA00,	// device
	IAP2_CTRL_MSGID_StopExternalAccessoryProtocolSession = 0xEA01,	// device
	IAP2_CTRL_MSGID_StatusExternalAccessoryProtocolSession = 0xEA03,	// accessory?
} iap2_ctrl_msgid_t;

typedef enum
{
	IAP2_IIID_Name = 0,			// utf8
	IAP2_IIID_ModelIdentifier,	// utf8
	IAP2_IIID_Manufacturer,		// utf8
	IAP2_IIID_SerialNumber,		// utf8
	IAP2_IIID_FirmwareVersion,	// utf8
	IAP2_IIID_HardwareVersion,	// utf8
	IAP2_IIID_MsgSentByAccessory,		// uint16[0+]
	IAP2_IIID_MsgReceivedByAccessory,	// uint16[0+]
	IAP2_IIID_PowerProvidingCapability,	// enum
	IAP2_IIID_MaximumCurrentDrawnFromDevice,	// uint16 (mA)
	IAP2_IIID_SupportedExternalAccessoryProtocol,	// group
	IAP2_IIID_AppTeamMatchID,	// utf8
	IAP2_IIID_CurrentLanguage,	// utf8
	IAP2_IIID_SupportedLanguage,	// utf8
	IAP2_IIID_USBDeviceTransportComponent = 15,	// group
	IAP2_IIID_USBHostTransportComponent,		// group
	IAP2_IIID_BluetoothTransportComponent,		// group
	IAP2_IIID_iAP2HIDComponent,
	IAP2_IIID_ProductPlanUID = 34,	// utf8	
} iap2_iiid_t;

// ExternalAccessoryProtocol
typedef enum
{
	IAP2_EAPID_ProtocolIdentifier = 0,	// uint8, unique per accessory
	IAP2_EAPID_ProtocolName,			// utf8
	IAP2_EAPID_ProtocolMatchAction,		// enum (iap2_match_action_t)
	IAP2_EAPID_NativeTransportComponentIdentifier,
} iap2_eapid_t;

typedef enum
{
	IAP2_TCID_ComponentIdentifier = 0,	// uint16, unique per accessory
	IAP2_TCID_ComponentName,			// utf8
	IAP2_TCID_SupportsiAP2Connection,	// none
	IAP2_TCID_USBDeviceSupportedAudioRate,	//enum (USDDevice only)
} iap2_tcid_t;

typedef enum
{
	IAP2_PPC_None = 0,
	IAP2_PPC_LightningReceptaclePassthrough,
	IAP2_PPC_Advanced,
} iap2_power_providing_capability_t;

typedef enum
{
	IAP2_MA_NoAction = 0,
	IAP2_MA_OptionalAction,
	IAP2_MA_NoAlert,
	IAP2_MA_NoCommunicationProtocol,
} iap2_match_action_t;

typedef struct __packed
{
	iap2_short_t SessionId;
	unsigned char Data[0];
} iap2_ea_sess_message_t;

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
	unsigned short ComponentId;	// NOTE: shall be != 0 for usb host 
} iap2_transport_t;

struct iap2_transport_driver
{
	bool (*Send)(iap2_transport_t *t, const unsigned char *data, unsigned length);
	bool (*SwitchRole)(iap2_transport_t *t);
	unsigned short (*Identify)(iap2_transport_t *t, iap2_control_parameters_t *params);
};

typedef struct
{
	iap2_transport_t *Transport;
	unsigned char Seq;
	unsigned char Ack;
	bool SyncDone;

	dispatcher_context_t DispatcherContext;
	event_t SyncEvent;
	fifo_t SyncFifo;

	// NOTE: first session in this array should be a control session i. e. Type == IAP2_SESSION_TYPE_CONTROL (0)
	iap2_link_session1_t Sessions[IAP2_MAX_SESSIONS];
} iap2_context_t;

typedef struct
{
	iap2_context_t *Context;
	io_entry_t IoEntry;
	dispatcher_t IoDispatcher;
	unsigned short CurrentSessionId;
	unsigned char ProtocolId;
	const char *Url;
	const char *Filename;
	void *DriverState;
} iap2_protocol_t;

void iap2_initialize();
bool iap2_protocol_create(iap2_protocol_t *p, const char *url, const char *filename);
bool iap2_transport_create(iap2_transport_t *t, const char *id, unsigned char unit, const iap2_transport_driver_t *driver);
bool iap2_start(iap2_transport_t *t);
bool iap2_input(iap2_transport_t *t, const unsigned char *packet, unsigned packet_length);
void iap2_stop();


// helper functions
void iap2_helper_init_parameters(iap2_control_parameters_t *params, void *buffer, unsigned short length);
void *iap2_helper_add_parameter(iap2_control_parameters_t *params, unsigned short id, unsigned short length);
void iap2_helper_add_param_string(iap2_control_parameters_t *params, unsigned short id, const char *str);


#endif // APPLE_IAP2_H
