#include "iap2.h"
#include "../iap_fid.h"
#include "../cp30.h"
//#include "iap_comm.h"
#include <support/misc/pools.h>
#include <kernel/dispatch.h>
#include <kernel/verbose.h>
#include <kernel/panic.h>
#include <string.h>
#include <stdio.h>

#ifdef IAP2_DEBUG
#define _verbose(level, ...) verbose(level, "iAP2", __VA_ARGS__)
#else
#define _verbose(level, ...) { /* nothing */ }
#endif

#define IAP2_MAX_PROTOCOL_COUNT 4
static iap2_protocol_t *_protocols[IAP2_MAX_PROTOCOL_COUNT];
static unsigned _protocol_count = 0;

#define THREAD_STACK 2048
static exos_thread_t _thread;
static unsigned char _stack[THREAD_STACK] __attribute__((aligned(16)));
static void *_service(void *arg);
static volatile bool _service_run = false;
static bool _service_exit = false;

#define IAP2_BUFFERS 5

typedef struct 
{ 
	node_t Node;
	unsigned short Payload;
	unsigned short Length;
	unsigned char Buffer[IAP2_BUFFER_SIZE];
} iap2_buffer_t;

static iap2_buffer_t _input[IAP2_BUFFERS];
static iap2_buffer_t _output[IAP2_BUFFERS];

static pool_t _input_pool, _output_pool;
static fifo_t _input_fifo, _output_fifo;
static event_t _input_event;

void iap2_initialize()
{
	static bool _init_done = false;
	if (!_init_done)
	{
		apple_cp30_initialize();
		_init_done = true;
	}
}

bool iap2_protocol_create(iap2_protocol_t *p, const char *url, const char *filename)
{
	ASSERT(p != NULL && url != NULL && filename != NULL, KERNEL_ERROR_NULL_POINTER);

	for(unsigned i = 0; i < _protocol_count; i++)
	{
		iap2_protocol_t *pp = _protocols[i];
		ASSERT(pp != NULL, KERNEL_ERROR_NULL_POINTER);
		if (pp == p)
			kernel_panic(KERNEL_ERROR_LIST_ALREADY_CONTAINS_NODE);
	}

	if (_protocol_count < IAP2_MAX_PROTOCOL_COUNT)
	{
		*p = (iap2_protocol_t) { .Url = url, .Filename = filename };
		_protocols[_protocol_count++] = p;
		return true;	
	}
	return false;
}

bool iap2_transport_create(iap2_transport_t *t, const char *id, unsigned char unit, const iap2_transport_driver_t *driver)
{
	t->Id = id;
	t->Driver = driver;
	t->Unit = unit;
	t->LinkParams = (iap2_link_params_t){ /* all zero */ };
	return true;
}

bool iap2_start(iap2_transport_t *t)
{
	ASSERT(t != NULL && t->Driver != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(!_service_run, KERNEL_ERROR_KERNEL_PANIC);
	// TODO: support several instances

	t->Transaction = 1;

	_verbose(VERBOSE_COMMENT, "starting thread...");

	pool_create(&_input_pool, (node_t *)_input, sizeof(iap2_buffer_t), IAP2_BUFFERS);
	exos_event_create(&_input_event, EXOS_EVENTF_AUTORESET);
	exos_fifo_create(&_input_fifo, &_input_event);

	pool_create(&_output_pool, (node_t *)_output, sizeof(iap2_buffer_t), IAP2_BUFFERS);

	_service_exit = false;
	_service_run = true;

	exos_thread_create(&_thread, 5, _stack, THREAD_STACK, _service, t);
	return true;
}

void iap2_stop()
{
	_verbose(VERBOSE_COMMENT, "stopping thread...");
	_service_exit = true;
	exos_thread_join(&_thread);

	ASSERT(_service_run == false, KERNEL_ERROR_KERNEL_PANIC);
	_verbose(VERBOSE_DEBUG, "stopped!");
}

static bool _send(iap2_transport_t *t, const unsigned char *data, unsigned length)
{
	const iap2_transport_driver_t *driver = t->Driver;
	ASSERT(driver != NULL && driver->Send != NULL, KERNEL_ERROR_NULL_POINTER);
	return driver->Send(t, data, length);
}

bool iap2_input(iap2_transport_t *t, const unsigned char *packet, unsigned packet_length)
{
	// FIXME: allow transport to allocate the buffer prior to filling it (zero copy)
	iap2_buffer_t *buf = (iap2_buffer_t *)pool_allocate(&_input_pool);
	if (buf != NULL)
	{
		ASSERT(packet_length < sizeof(buf->Buffer), KERNEL_ERROR_KERNEL_PANIC);
		memcpy(buf->Buffer, packet, packet_length);
		buf->Length = packet_length;

		exos_fifo_queue(&_input_fifo, &buf->Node);
		return true;
	}
	else 
	{
		_verbose(VERBOSE_ERROR, "cannot allocate input buffer");
		return false;
	}
}


static iap2_buffer_t *_wait_buffer(iap2_transport_t *t, unsigned timeout)
{
	iap2_buffer_t *buf = (iap2_buffer_t *)exos_fifo_wait(&_input_fifo, timeout);
	return buf;
}

static void _free_input_buffer(iap2_buffer_t *buf)
{
	pool_free(&_input_pool, &buf->Node);
}

static bool _initialize(iap2_transport_t *t)
{
	// NOTE: this is iAP1 packet (length=2, lingo=0, cmd=0xee) 
	const unsigned char _hello[] = { 0xff, 0x55, 0x02, 0x00, 0xee, 0x10 };

	bool done = false;
	for(unsigned i = 0; i < 30; i++)
	{
		if (_send(t, _hello, sizeof(_hello)))
		{
			_verbose(VERBOSE_COMMENT, "sent hello...");
			iap2_buffer_t *buf = _wait_buffer(t, 1000);
			if (buf != NULL)
			{
				done = true;
				for (unsigned i = 0; i < sizeof(_hello); i++)
					if (_hello[i] != buf->Buffer[i]) { done = false; break; }

				_free_input_buffer(buf);
				break;
			}
		}
		else
		{
			_verbose(VERBOSE_ERROR, "send hello failed!");
			break;
		}
	}

	return done;
}

static unsigned char _checksum(unsigned char *buffer, unsigned length)
{
	unsigned char sum = 0;
	for (unsigned i = 0; i < length; i++) sum += buffer[i];
	return (unsigned char)(0x100 - sum);
}

static bool _parse_buffer(iap2_buffer_t *buf, iap2_header_t **pheader, unsigned short *poffset, unsigned short *plength)
{
	if (buf->Length < sizeof(iap2_header_t))
	{
		_verbose(VERBOSE_ERROR, "packet header incomplete, length = %d", buf->Length);
		return false;
	}

	iap2_header_t *hdr = (iap2_header_t *)buf->Buffer;
	unsigned short sop = IAP2SHTOH(hdr->Sop);
	unsigned short pkt_len = IAP2SHTOH(hdr->PacketLength);
	if (sop != 0xff5a || pkt_len > buf->Length)
	{
		_verbose(VERBOSE_ERROR, "bad packet header", sop);
		return false;
	}

	unsigned char checksum = _checksum((unsigned char *)hdr, sizeof(iap2_header_t) - 1);
	if (checksum != hdr->Checksum)
	{
		_verbose(VERBOSE_ERROR, "packet header bad checksum ($%02x != $%02x)", hdr->Checksum, checksum);
		return false;
	}

	*pheader = hdr;

	if (pkt_len >= (sizeof(iap2_header_t) + 2)) // NOTE: at least 1 byte of payload + checksum
	{
		unsigned short payload_offset = sizeof(iap2_header_t);
		unsigned short payload_len = (pkt_len - payload_offset) - 1;
		unsigned char *payload = buf->Buffer + payload_offset;
		checksum = _checksum(payload, payload_len);
		if (checksum == payload[payload_len])
		{
			*poffset = payload_offset;
			*plength = payload_len;
			return true;
		}
	}
	else
	{
		*poffset = 0;
		*plength = 0;
		return true;
	}

	_verbose(VERBOSE_ERROR, "bad packet length");
	return false;
}

static iap2_buffer_t *_init_packet(iap2_context_t *iap2, unsigned char control, unsigned char sess_id)
{
	iap2_buffer_t *buf = (iap2_buffer_t *)pool_allocate(&_output_pool);
	if (buf != NULL)
	{
		iap2_header_t *hdr = (iap2_header_t *)buf->Buffer;

		*hdr = (iap2_header_t) { .Sop = HTOIAP2S(0xff5a),
			.PacketLength = HTOIAP2S(sizeof(iap2_header_t)),
			.Control = control, .SessionId = sess_id,
			.SequenceNumber = iap2->Seq, .AckNumber = iap2->Ack };

		buf->Length = buf->Payload = sizeof(iap2_header_t);
		return buf;
	}
	return NULL;
}

static bool _send_packet(iap2_context_t *iap2, iap2_buffer_t *buf)
{
	iap2_header_t *hdr = (iap2_header_t *)buf->Buffer;
	hdr->PacketLength = HTOIAP2S(buf->Length);
	hdr->Checksum = _checksum((unsigned char *)hdr, sizeof(iap2_header_t) - 1);

	_verbose(VERBOSE_DEBUG, "send packet [sess #%d] ctrl=$%02x, seq=$%02x, packet_length=%d", 
		hdr->SessionId, hdr->Control, hdr->SequenceNumber, IAP2SHTOH(hdr->PacketLength));

	return _send(iap2->Transport, buf->Buffer, buf->Length);
}

static void _send_ack(iap2_context_t *iap2)
{
	iap2_buffer_t *buf = _init_packet(iap2, IAP2_CONTROLF_ACK, 0);
	if (buf != NULL)
	{
		_send_packet(iap2, buf);

		pool_free(&_output_pool, &buf->Node);	// FIXME: we should not dispose the buffer if it has been queued for output
	}
}

static void _sync_callback(dispatcher_context_t *context, dispatcher_t *dispatcher)
{
	iap2_context_t *iap2 = (iap2_context_t *)dispatcher->CallbackState;
	iap2_transport_t *t = iap2->Transport;
	iap2_link_params_t *link = &t->LinkParams;
	unsigned char ctrl = IAP2_CONTROLF_SYN;

	iap2_buffer_t *input = (iap2_buffer_t *)exos_fifo_dequeue(&iap2->SyncFifo);
	if (input != NULL)
	{
		//_verbose(VERBOSE_DEBUG, "[sync] rx [+%d] %d bytes!", input->Payload, input->Length);

		iap2_link_sync_payload1_t *sync1 = (iap2_link_sync_payload1_t *)(input->Buffer + input->Payload);
		if (sync1->LinkVersion == 0x01)
		{
			unsigned short max_pkt_len = IAP2SHTOH(sync1->MaxRcvPacketLength);
			_verbose(VERBOSE_DEBUG, "rx [sync] max_out_pks=%d, rcv_pkt_len=%d", 
				sync1->MaxNumOutstandingPackets, max_pkt_len);
			_verbose(VERBOSE_DEBUG, "rx [sync] rtx=%d (%d ms) ack=%d (%d ms)",
				sync1->MaxRetransmits, IAP2SHTOH(sync1->RetransmitTimeout), sync1->MaxCumulativeAcks, IAP2SHTOH(sync1->CumulativeAckTimeout));
		
			if (sync1->MaxNumOutstandingPackets >= link->MaxNumOutstandingPackets && 
				max_pkt_len >= link->MaxNumOutstandingPackets)
			{
				ctrl = 0;	// remove SYN flag i.e. OK
				_verbose(VERBOSE_COMMENT, "[sync] parameters agreement");
			}

			ctrl |= IAP2_CONTROLF_ACK;
		}
		else _verbose(VERBOSE_ERROR, "[sync] rx unk link version %d != 1!", sync1->LinkVersion);

		pool_free(&_input_pool, &input->Node);
	}

	iap2_buffer_t *buf = _init_packet(iap2, ctrl, 0);	// SYN, sess=0
	if (buf != NULL)
	{
		if (ctrl & IAP2_CONTROLF_SYN)
		{
			// TODO

			iap2_link_sync_payload1_t *sync2 = (iap2_link_sync_payload1_t *)(buf->Buffer + buf->Length);
			sync2->LinkVersion = 0x01;

			sync2->MaxNumOutstandingPackets = link->MaxNumOutstandingPackets;	// IAP2_BUFFERS
			sync2->MaxRcvPacketLength = HTOIAP2S(link->MaxRcvPacketLength);		// IAP2_BUFFER_SIZE
			sync2->RetransmitTimeout = HTOIAP2S(link->RetransmitTimeout);		// transport init
			sync2->CumulativeAckTimeout = HTOIAP2S(link->CumulativeAckTimeout);	// transport init
			sync2->MaxRetransmits = link->MaxRetransmits;		// transport init
			sync2->MaxCumulativeAcks = link->MaxCumulativeAcks;	// transport init

			buf->Length += sizeof(iap2_link_sync_payload1_t);
			for(unsigned i = 0; i < IAP2_MAX_SESSIONS; i++)	// FIXME: replace constant with a context variable
			{
				sync2->Sessions[i] = iap2->Sessions[i];
				buf->Length += sizeof(iap2_link_session1_t);
			}

			buf->Buffer[buf->Length] = _checksum((unsigned char *)sync2, buf->Length - sizeof(iap2_header_t));
			buf->Length++;

			_verbose(VERBOSE_DEBUG, "tx [sync] max_out_pks=%d, rcv_pkt_len=%d", 
				sync2->MaxNumOutstandingPackets, IAP2SHTOH(sync2->MaxRcvPacketLength));
			_verbose(VERBOSE_DEBUG, "tx [sync] rtx=%d (%d ms) ack=%d (%d ms)",
				sync2->MaxRetransmits, IAP2SHTOH(sync2->RetransmitTimeout), sync2->MaxCumulativeAcks, IAP2SHTOH(sync2->CumulativeAckTimeout));
		}

		_send_packet(iap2, buf);
		
		pool_free(&_output_pool, &buf->Node);	// FIXME: we should not dispose the buffer if it has been queued for output
	}
	else _verbose(VERBOSE_ERROR, "[sync] cannot allocate output buffer!");

	if (ctrl & IAP2_CONTROLF_SYN)	// NOTE: skip when agreement have been reached
	{
		exos_dispatcher_add(context, dispatcher, EXOS_TIMEOUT_NEVER);	// FIXME: use retransmit?
	}
	else 
	{
		// NOTE:don't send more messages to this dispatcher!
		iap2->SyncDone = true;
	}
}

static iap2_control_session_message_parameter_t *_find_param(iap2_control_sess_message_t *ctrl_msg, unsigned short id)
{
	ASSERT(ctrl_msg != NULL, KERNEL_ERROR_NULL_POINTER);
	unsigned msg_len = IAP2SHTOH(ctrl_msg->MessageLength);
	unsigned offset = sizeof(iap2_control_sess_message_t);
	while(offset < msg_len)
	{
		iap2_control_session_message_parameter_t *p = (iap2_control_session_message_parameter_t *)
			((void *)ctrl_msg + offset);
		unsigned short pid = IAP2SHTOH(p->Id); 
		if (pid == id)
			return p;
		
		offset += IAP2SHTOH(p->Length);
	}
	return NULL;
}

static void _debug_params(iap2_control_sess_message_t *ctrl_msg)
{
	ASSERT(ctrl_msg != NULL, KERNEL_ERROR_NULL_POINTER);
	unsigned msg_len = IAP2SHTOH(ctrl_msg->MessageLength);
	unsigned offset = sizeof(iap2_control_sess_message_t);
	while(offset < msg_len)
	{
		static char hex_buf[80];
		void *pp = (void *)ctrl_msg + offset;
		iap2_control_session_message_parameter_t *p = (iap2_control_session_message_parameter_t *)pp;
		unsigned short pid = IAP2SHTOH(p->Id);
		unsigned short plen = IAP2SHTOH(p->Length);
		
		char *hex_ptr = hex_buf;
		*hex_ptr = '\0';
		unsigned payload = plen - sizeof(iap2_control_session_message_parameter_t); 
		unsigned pay_off = 0;
		for(unsigned i = 0; i < payload; i++)
		{
			hex_ptr += sprintf(hex_ptr, "%02x ", p->Data[pay_off++]);
			if (i == 24)
			{
				hex_ptr += sprintf(hex_ptr, "...");
				break;
			}
		}
		ASSERT(hex_ptr < hex_buf + sizeof(hex_buf), KERNEL_ERROR_KERNEL_PANIC);
		_verbose(VERBOSE_ERROR, "id=%d %s", pid, hex_buf);
		offset += plen;
	}
}

static iap2_control_sess_message_t *_init_control_msg(iap2_buffer_t *buf, unsigned short msgid)
{
	iap2_control_sess_message_t *ctrl_msg = (iap2_control_sess_message_t *)(buf->Buffer + buf->Payload);
	ctrl_msg->Sop = HTOIAP2S(IAP2_CTRL_SOF);
	ctrl_msg->MessageLength = HTOIAP2S(sizeof(iap2_control_sess_message_t));	// NOTE: header only
	ctrl_msg->MessageId = HTOIAP2S(msgid);
	return ctrl_msg;
}

static void *_add_parameter(iap2_control_sess_message_t *ctrl_msg, unsigned short id, unsigned short length)
{
	unsigned short msg_length = IAP2SHTOH(ctrl_msg->MessageLength);
	void *ptr = (void *)ctrl_msg + msg_length;
	iap2_control_session_message_parameter_t *param = (iap2_control_session_message_parameter_t *)ptr;
	unsigned short param_length = sizeof(iap2_control_session_message_parameter_t) + length;
	param->Length = HTOIAP2S(param_length);
	param->Id = HTOIAP2S(id);
	ctrl_msg->MessageLength = HTOIAP2S(msg_length + param_length);
	return ptr + sizeof(iap2_control_session_message_parameter_t);
}

static void _add_param_string(iap2_control_sess_message_t *ctrl_msg, unsigned short id, const char *str)
{
	void *payload = _add_parameter(ctrl_msg, id, strlen(str) + 1);
	strcpy((char *)payload, str);
}

static void _send_auth_certificate(iap2_context_t *iap2)
{
	iap2_link_session1_t *sess0 = &iap2->Sessions[0];
	ASSERT(sess0->Type == IAP2_SESSION_TYPE_CONTROL, KERNEL_ERROR_KERNEL_PANIC);

	iap2_buffer_t *buf = _init_packet(iap2, IAP2_CONTROLF_ACK, sess0->Id);
	if (buf != NULL)
	{
		iap2_control_sess_message_t *ctrl_msg = _init_control_msg(buf, IAP2_CTRL_MSGID_AuthenticationCertificate);
		
		unsigned short cert_size;
		if (apple_cp30_read_acc_cert_length(&cert_size))
		{
			ASSERT(cert_size < 800, KERNEL_ERROR_KERNEL_PANIC);
			void *payload = _add_parameter(ctrl_msg, 0, cert_size);

			if (apple_cp30_read_acc_cert(payload, cert_size))
			{
				buf->Length += IAP2SHTOH(ctrl_msg->MessageLength);	// NOTE: fix buffer length 

				buf->Buffer[buf->Length] = _checksum((unsigned char *)ctrl_msg, buf->Length - sizeof(iap2_header_t));
				buf->Length++;

				_verbose(VERBOSE_DEBUG, "tx [auth] acc certificate (%d), msg_length=%d", cert_size, IAP2SHTOH(ctrl_msg->MessageLength));

				_send_packet(iap2, buf);
			}
			else _verbose(VERBOSE_ERROR, "[auth] cannot read cert data from CP!");
		}
		else _verbose(VERBOSE_ERROR, "[auth] cannot read cert length from CP!");

		pool_free(&_output_pool, &buf->Node);	// FIXME: we should not dispose the buffer if it has been queued for output
	}
	else _verbose(VERBOSE_ERROR, "[auth] cannot allocate output buffer!");
}

static void _send_auth_challenge_resp(iap2_context_t *iap2, iap2_control_sess_message_t *req_msg)
{
	iap2_link_session1_t *sess0 = &iap2->Sessions[0];
	ASSERT(sess0->Type == IAP2_SESSION_TYPE_CONTROL, KERNEL_ERROR_KERNEL_PANIC);

	iap2_control_session_message_parameter_t *ch = _find_param(req_msg, 0);
	if (ch != NULL)
	{
		unsigned short ch_len = IAP2SHTOH(ch->Length);
		if (ch_len == 36)
		{
			iap2_buffer_t *buf = _init_packet(iap2, IAP2_CONTROLF_ACK, sess0->Id);
			if (buf != NULL)
			{
				iap2_control_sess_message_t *resp_msg = _init_control_msg(buf, IAP2_CTRL_MSGID_AuthenticationResponse);

				unsigned short resp_size;
				if (apple_cp30_begin_challenge(ch->Data, ch_len - sizeof(iap2_control_session_message_parameter_t),
					&resp_size))
				{
					void *payload = _add_parameter(resp_msg, 0, resp_size);
					if (apple_cp30_read_challenge_resp(payload, resp_size))
					{
						buf->Length += IAP2SHTOH(resp_msg->MessageLength);	// NOTE: fix buffer length 

						buf->Buffer[buf->Length] = _checksum((unsigned char *)resp_msg, buf->Length - sizeof(iap2_header_t));
						buf->Length++;
						_send_packet(iap2, buf);
					}
					else _verbose(VERBOSE_ERROR, "[auth] CP challenge response read failure");
				}
				else _verbose(VERBOSE_ERROR, "[auth] CP challenge failed");
				
				pool_free(&_output_pool, &buf->Node);	// FIXME: we should not dispose the buffer if it has been queued for output
			}
		}
		else _verbose(VERBOSE_ERROR, "[auth] bad challenge param length (%d)", ch_len);
	} else _verbose(VERBOSE_ERROR, "[auth] challenge param not found!");
}

static unsigned short _sent_msgs[] = { 
	/*IAP2_CTRL_MSGID_StartExternalAccessoryProtocolSession,
	IAP2_CTRL_MSGID_StopExternalAccessoryProtocolSession*/ };
static unsigned short _rcvd_msgs[] = { 
	IAP2_CTRL_MSGID_StartExternalAccessoryProtocolSession,
	IAP2_CTRL_MSGID_StopExternalAccessoryProtocolSession };

#ifndef IAP2_ACCESORY_NAME
#define IAP2_ACCESORY_NAME "MIO Labs Test Accesory"
#endif
#ifndef IAP2_ACCESORY_MODEL
#define IAP2_ACCESORY_MODEL "undefined"
#endif
#ifndef IAP2_ACCESORY_SERIAL
#define IAP2_ACCESORY_SERIAL "00000"
#endif
#ifndef IAP2_ACCESORY_FW_VERSION
#define IAP2_ACCESORY_FW_VERSION "1.0"
#endif
#ifndef IAP2_ACCESORY_HW_VERSION
#define IAP2_ACCESORY_HW_VERSION "prototype"
#endif
#ifndef IAP2_ACCESORY_MANUFACTURER
#define IAP2_ACCESORY_MANUFACTURER "MIO Research Labs"
#endif

static unsigned short _get_transport_component_id(iap2_transport_t *t, unsigned short *pcid, unsigned char *param_buf, unsigned buf_size)
{
	iap2_control_parameters_t params;
	iap2_helper_init_parameters(&params, param_buf, buf_size);

	const iap2_transport_driver_t *driver = t->Driver;
	ASSERT(driver != NULL && driver->Identify != NULL, KERNEL_ERROR_NULL_POINTER);
	// NOTE: transport must return component id (group) on success or zero for failure. i.e. IAP2_IIID_USBDeviceTransportComponent
	unsigned short cid = driver->Identify(t, &params);
	if (cid != 0)
	{
		*pcid = cid;
		return params.Length;
	}
	return 0;
}

static unsigned short _get_protocol_definition(iap2_context_t *iap2, unsigned index, unsigned char *param_buf, unsigned buf_size)
{
	// NOTE: fill ExternalAccessoryProtocol group...
	iap2_control_parameters_t params;
	iap2_helper_init_parameters(&params, param_buf, buf_size);

	ASSERT(index < IAP2_MAX_PROTOCOL_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	if (index < _protocol_count)
	{
		iap2_protocol_t *p = _protocols[index];
		ASSERT(p != NULL, KERNEL_ERROR_NULL_POINTER);
		p->ProtocolId = index + 0xa0;	// NOTE: each protocol shall be unique

		unsigned char *id = iap2_helper_add_parameter(&params, IAP2_EAPID_ProtocolIdentifier, 1);
		*id = p->ProtocolId;

		ASSERT(p->Url != NULL && p->Filename, KERNEL_ERROR_NULL_POINTER);
		iap2_helper_add_param_string(&params, IAP2_EAPID_ProtocolName, p->Url);

		unsigned char *ma = iap2_helper_add_parameter(&params, IAP2_EAPID_ProtocolMatchAction, 1);
		*ma = IAP2_MA_NoAlert;	// FIXME: configurable

		//iap2_short_t *tcid = iap2_helper_add_parameter(&params, IAP2_EAPID_NativeTransportComponentIdentifier, sizeof(iap2_short_t));
		//*tcid = HTOIAP2S(iap2->Transport->ComponentId);

//		_verbose(VERBOSE_DEBUG, "protocol id='%d' name='%s'", p->ProtocolId, p->Url);
	}
	return params.Length;
}

static void _ea_dispatch(dispatcher_context_t *context, dispatcher_t *dispatcher)
{
	static unsigned char buffer[16];
	iap2_protocol_t *p = (iap2_protocol_t *)dispatcher->CallbackState;
	ASSERT(p != NULL, KERNEL_ERROR_NULL_POINTER);
	iap2_context_t *iap2 = (iap2_context_t *)p->Context;
	ASSERT(iap2 != NULL, KERNEL_ERROR_NULL_POINTER);
	iap2_link_session1_t *sess1 = &iap2->Sessions[1];
	ASSERT(sess1->Type == IAP2_SESSION_TYPE_EXTERNAL_ACCESSORY, KERNEL_ERROR_KERNEL_PANIC);

	while(1)
	{
		unsigned offset = 0;
		unsigned rem = 256;	// FIXME: IAP2_BUFFER_SIZE - (header + footer)

		iap2_buffer_t *buf = _init_packet(iap2, IAP2_CONTROLF_ACK, sess1->Id);
		if (buf != NULL)
		{
			iap2_ea_sess_message_t *ea_msg = (iap2_ea_sess_message_t *)(buf->Buffer + buf->Payload);
			ea_msg->SessionId = HTOIAP2S(p->CurrentSessionId);
			while(rem != 0)
			{
				unsigned done = exos_io_read(&p->IoEntry, &ea_msg->Data[offset], rem);
				if (done <= 0) break;
				offset += done;
				rem -= done;
			}
			_verbose(VERBOSE_DEBUG, "ea %s got %d bytes -> send_packet", p->Url, offset);					

			buf->Length += (sizeof(iap2_ea_sess_message_t) + offset);
			buf->Buffer[buf->Length] = _checksum((unsigned char *)ea_msg, buf->Length - sizeof(iap2_header_t));
			buf->Length++;
			_send_packet(iap2, buf);

			pool_free(&_output_pool, &buf->Node);	// FIXME: we should not dispose the buffer if it has been queued for output
		}
		
		/*if (rem == 0)*/ break;
	}
	exos_dispatcher_add(context, dispatcher, EXOS_TIMEOUT_NEVER);
}

static bool _start_ea_session(unsigned short sess_id, unsigned char proto_id, iap2_context_t *iap2)
{
	ASSERT(_protocol_count <= IAP2_MAX_PROTOCOL_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	iap2_protocol_t *p = NULL;
	for (unsigned index	= 0; index < _protocol_count; index++)
	{
		if (_protocols[index]->ProtocolId == proto_id)
		{
			p = _protocols[index];
			break;
		}
	}
	
	if (p != NULL)
	{
		if (p->CurrentSessionId == 0)
		{
			io_error_t err = exos_io_open_path(&p->IoEntry, p->Filename, IOF_WRITE);
			if (err == IO_OK)
			{
				ASSERT(sess_id != 0, KERNEL_ERROR_KERNEL_PANIC);
				p->CurrentSessionId = sess_id;
				p->Context = iap2;

				exos_dispatcher_create(&p->IoDispatcher, &p->IoEntry.InputEvent, _ea_dispatch, p);
				exos_dispatcher_add(&iap2->DispatcherContext, &p->IoDispatcher, EXOS_TIMEOUT_NEVER);
			}
			else _verbose(VERBOSE_ERROR, "StartEAProtocolSession(): can't open stream file '%s'", p->Filename);
		}
		else _verbose(VERBOSE_ERROR, "StartEAProtocolSession(): protocol '%s' already busy", p->Url);
	}
	else _verbose(VERBOSE_ERROR, "StartEAProtocolSession(): unk protocol_id $%x!", proto_id);
	return false;
}

static bool _find_ea_sess_protocol(unsigned short sess_id, iap2_protocol_t **pp)
{
	ASSERT(_protocol_count <= IAP2_MAX_PROTOCOL_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	iap2_protocol_t *p = NULL;
	for (unsigned index	= 0; index < _protocol_count; index++)
	{
		if (_protocols[index]->CurrentSessionId == sess_id)
		{
			p = _protocols[index];
			break;
		}
	}
	if (p != NULL)
	{
		*pp = p;
		return true;
	}
	return false;
}

static void _stop_ea_session(unsigned short sess_id)
{
	ASSERT(sess_id != 0, KERNEL_ERROR_KERNEL_PANIC);

	iap2_protocol_t *p = NULL;
	if (_find_ea_sess_protocol(sess_id, &p))
	{
		ASSERT(p != NULL, KERNEL_ERROR_NULL_POINTER);
		p->CurrentSessionId = 0;
		
		iap2_context_t *iap2 = p->Context;
		ASSERT(iap2 != NULL, KERNEL_ERROR_KERNEL_PANIC);
		exos_dispatcher_remove(&iap2->DispatcherContext, &p->IoDispatcher);
		p->Context = NULL;

		exos_io_close(&p->IoEntry); //NOTE: close stream
	}
	else _verbose(VERBOSE_ERROR, "StopEAProtocolSession(): unk session_id $%x", sess_id);
}


static void _send_identification(iap2_context_t *iap2)
{
	iap2_link_session1_t *sess0 = &iap2->Sessions[0];
	ASSERT(sess0->Type == IAP2_SESSION_TYPE_CONTROL, KERNEL_ERROR_KERNEL_PANIC);
	unsigned char param_buffer[64];

	iap2_buffer_t *buf = _init_packet(iap2, IAP2_CONTROLF_ACK, sess0->Id);
	if (buf != NULL)
	{
		iap2_control_sess_message_t *resp_msg = _init_control_msg(buf, IAP2_CTRL_MSGID_IdentificationInformation);

		_add_param_string(resp_msg, IAP2_IIID_Name, IAP2_ACCESORY_NAME);
		_add_param_string(resp_msg, IAP2_IIID_ModelIdentifier, IAP2_ACCESORY_MODEL);
		_add_param_string(resp_msg, IAP2_IIID_Manufacturer, IAP2_ACCESORY_MANUFACTURER);
		_add_param_string(resp_msg, IAP2_IIID_SerialNumber, IAP2_ACCESORY_SERIAL);
		_add_param_string(resp_msg, IAP2_IIID_FirmwareVersion, IAP2_ACCESORY_FW_VERSION);
		_add_param_string(resp_msg, IAP2_IIID_HardwareVersion, IAP2_ACCESORY_HW_VERSION);

		iap2_short_t *stm = _add_parameter(resp_msg, IAP2_IIID_MsgSentByAccessory, sizeof(_sent_msgs));
		for (unsigned i = 0; i < (sizeof(_sent_msgs) / sizeof(iap2_short_t)); i++) stm[i] = HTOIAP2S(_sent_msgs[i]); 
		iap2_short_t *rdm = _add_parameter(resp_msg, IAP2_IIID_MsgReceivedByAccessory, sizeof(_rcvd_msgs));
		for (unsigned i = 0; i < (sizeof(_rcvd_msgs) / sizeof(iap2_short_t)); i++) rdm[i] = HTOIAP2S(_rcvd_msgs[i]); 

		unsigned char *ppc = _add_parameter(resp_msg, IAP2_IIID_PowerProvidingCapability, 1);
		*ppc = IAP2_PPC_None;	// FIXME

		iap2_short_t *mcd = _add_parameter(resp_msg, IAP2_IIID_MaximumCurrentDrawnFromDevice, sizeof(unsigned short));
		*mcd = HTOIAP2S(0);	// FIXME

		// SupportedExternalAccessoryProtocol (0+)
		for(unsigned pi = 0; pi < IAP2_MAX_PROTOCOL_COUNT; pi++)
		{
			unsigned short eap_len = _get_protocol_definition(iap2, pi, param_buffer, sizeof(param_buffer));
			if (eap_len != 0)
			{
				void *eap = _add_parameter(resp_msg, IAP2_IIID_SupportedExternalAccessoryProtocol, eap_len);
				memcpy(eap, param_buffer, eap_len);
			} 
			else 
			{
				_verbose(VERBOSE_DEBUG, "identification reporting %d accesory protocols", pi);
				break;
			}
		}

		// TODO AppMatchTeamID

		_add_param_string(resp_msg, IAP2_IIID_CurrentLanguage, "en");
		_add_param_string(resp_msg, IAP2_IIID_SupportedLanguage, "en");

		unsigned short tgc_iiid = 0;
		unsigned short tgc_len = _get_transport_component_id(iap2->Transport, &tgc_iiid, param_buffer, sizeof(param_buffer));
		ASSERT(tgc_len != 0, KERNEL_ERROR_KERNEL_PANIC);
		ASSERT(tgc_iiid != 0, KERNEL_ERROR_KERNEL_PANIC);
		_verbose(VERBOSE_DEBUG, "transport component id=%d", tgc_iiid);
		void *tgc = _add_parameter(resp_msg, tgc_iiid, tgc_len);
		memcpy(tgc, param_buffer, tgc_len);
		
		_add_param_string(resp_msg, IAP2_IIID_ProductPlanUID, IAP2_PRODUCT_PLAN_UID);

#ifdef DEBUG
//		_debug_params(resp_msg);
#endif
		buf->Length += IAP2SHTOH(resp_msg->MessageLength);	// NOTE: fix buffer length 
		buf->Buffer[buf->Length] = _checksum((unsigned char *)resp_msg, buf->Length - sizeof(iap2_header_t));
		buf->Length++;
		_send_packet(iap2, buf);
		
		pool_free(&_output_pool, &buf->Node);	// FIXME: we should not dispose the buffer if it has been queued for output
	}
}

static void _parse_control(iap2_context_t *iap2, iap2_control_sess_message_t *ctrl_msg)
{
	unsigned msg_id = IAP2SHTOH(ctrl_msg->MessageId);
	unsigned msg_len = IAP2SHTOH(ctrl_msg->MessageLength); 
	unsigned offset = sizeof(iap2_control_sess_message_t);
	int payload = msg_len - sizeof(iap2_control_sess_message_t);

	iap2_control_session_message_parameter_t *param;
	int protocol, session;

	switch(msg_id)
	{
		case IAP2_CTRL_MSGID_RequestAuthenticationCertificate:
			_verbose(VERBOSE_DEBUG, "got ctrl RequestAuthenticationCertificate");
			_send_auth_certificate(iap2);
			break;
		case IAP2_CTRL_MSGID_RequestAuthenticationChallengeResponse:
			_verbose(VERBOSE_DEBUG, "got ctrl RequestAuthenticationChallengeResponse (%d bytes)", payload);
			if (payload > 0) _send_auth_challenge_resp(iap2, ctrl_msg);
			else _verbose(VERBOSE_ERROR, "incorrect challenge size!");
			break;
		case IAP2_CTRL_MSGID_AuthenticationFailed:
			_verbose(VERBOSE_DEBUG, "got ctrl AuthenticationFailed (ERROR)");
			// NOTE (should not happen)
			_send_ack(iap2);
			break;
		case IAP2_CTRL_MSGID_AuthenticationSucceeded:
			_verbose(VERBOSE_DEBUG, "got ctrl AuthenticationSucceeded (OK)");
			// NOTE: device proceeds with identification now 
			break;

		case IAP2_CTRL_MSGID_StartIdentification:
			_verbose(VERBOSE_DEBUG, "got ctrl StartIdentification");
			_send_identification(iap2);
			break;
		case IAP2_CTRL_MSGID_IdentificationRejected:
			_verbose(VERBOSE_ERROR, "got ctrl IdentificationRejected (ERROR)");
			// NOTE (should not happen)
#ifdef DEBUG
			_debug_params(ctrl_msg);
#endif
			_send_ack(iap2);
			break;
		case IAP2_CTRL_MSGID_IdentificationAccepted:
			_verbose(VERBOSE_DEBUG, "got ctrl IdentificationAccepted (OK)");
			_send_ack(iap2);
			break;

		case IAP2_CTRL_MSGID_StartExternalAccessoryProtocolSession:
			param = _find_param(ctrl_msg, 0);
			protocol = (param != NULL) ? *(unsigned char *)param->Data : -1;
			param = _find_param(ctrl_msg, 1);
			session = (param != NULL) ? IAP2SHTOH(*(iap2_short_t *)param->Data) : -1;
			 
			_verbose(VERBOSE_DEBUG, "got ctrl StartEAProtocolSession, protocol=%02x, session=$%04x",
				protocol, session);
			_start_ea_session(session, protocol, iap2);
			_send_ack(iap2);
			break;
		case IAP2_CTRL_MSGID_StopExternalAccessoryProtocolSession:
			param = _find_param(ctrl_msg, 0);
			session = (param != NULL) ? IAP2SHTOH(*(iap2_short_t *)param->Data) : -1;
			 
			_verbose(VERBOSE_DEBUG, "got ctrl StopEAProtocolSession, session=$%04x",
				session);
			_stop_ea_session(session);
			_send_ack(iap2);
			break;


		default:
			_verbose(VERBOSE_ERROR, "got unk control msg id=$%04x", msg_id);
			break;
	}
}

static void _rx_callback(dispatcher_context_t *context, dispatcher_t *dispatcher)
{
	iap2_context_t *iap2 = (iap2_context_t *)dispatcher->CallbackState;
	iap2_buffer_t *buf;
	while(buf = (iap2_buffer_t *)exos_fifo_dequeue(&_input_fifo), buf != NULL)
	{
		iap2_header_t *hdr;
		unsigned short payload_offset;
		unsigned short payload_len;
		if (_parse_buffer(buf, &hdr, &payload_offset, &payload_len))
		{
			_verbose(VERBOSE_DEBUG, "got packet [sess #%d] ctrl=$%02x, seq=$%02x, ack=$%02x, payload=%d bytes",
				hdr->SessionId, hdr->Control, hdr->SequenceNumber, hdr->AckNumber, payload_len);

			if (hdr->Control & IAP2_CONTROLF_ACK)
			{
				if (hdr->AckNumber == iap2->Seq)
					iap2->Seq++;
			}
		
			buf->Payload = payload_offset;
			buf->Length = payload_len;

			if (hdr->SessionId == 0)
			{
				iap2->Ack = hdr->SequenceNumber;
				if (!iap2->SyncDone)
				{
					exos_fifo_queue(&iap2->SyncFifo, &buf->Node);
				}
				else pool_free(&_input_pool, &buf->Node);
			}
			else 
			{
				iap2_link_session1_t *sess0 = &iap2->Sessions[0];
				ASSERT(sess0->Type == IAP2_SESSION_TYPE_CONTROL, KERNEL_ERROR_KERNEL_PANIC);
				iap2_link_session1_t *sess1 = &iap2->Sessions[1];
				ASSERT(sess1->Type == IAP2_SESSION_TYPE_EXTERNAL_ACCESSORY, KERNEL_ERROR_KERNEL_PANIC);

				if (hdr->SessionId == sess0->Id)
				{
					iap2->Ack = hdr->SequenceNumber;
					
					iap2_control_sess_message_t *ctrl_msg = (iap2_control_sess_message_t *)(buf->Buffer + buf->Payload);
					unsigned msg_len = IAP2SHTOH(ctrl_msg->MessageLength);
					if (msg_len <= buf->Length && msg_len >= sizeof(iap2_control_sess_message_t))
					{
						//_verbose(VERBOSE_DEBUG, "got control message Id=$%04x (%d bytes)", IAP2SHTOH(ctrl_msg->MessageId), msg_len);
						_parse_control(iap2, ctrl_msg);
					}
					else _verbose(VERBOSE_ERROR, "control message discarded (bad length)");
				}
				else if (hdr->SessionId == sess1->Id)
				{
					iap2->Ack = hdr->SequenceNumber;

					iap2_ea_sess_message_t *ea_msg = (iap2_ea_sess_message_t *)(buf->Buffer + buf->Payload);
					unsigned msg_len = payload_len - sizeof(iap2_ea_sess_message_t);
					if (payload_len >= sizeof(iap2_ea_sess_message_t))
					{
						unsigned short ea_sess_id = IAP2SHTOH(ea_msg->SessionId);
						//_verbose(VERBOSE_COMMENT, "got ea message, ea_sid=$%04x (%d bytes)", ea_sess_id, msg_len);
						
						iap2_protocol_t *eap = NULL;
						if (_find_ea_sess_protocol(ea_sess_id, &eap))
						{
							ASSERT(eap != NULL, KERNEL_ERROR_NULL_POINTER);
							exos_io_write(&eap->IoEntry, ea_msg->Data, msg_len);
						}
						else _verbose(VERBOSE_ERROR, "can't relay ea message data (ea_sid=$%04x, %d bytes)", ea_sess_id, msg_len);
					}
					else _verbose(VERBOSE_ERROR, "ea message discarded (bad length)");

					_send_ack(iap2);
				}
				else
				{
					// TODO
					_verbose(VERBOSE_ERROR, "rx packet discarded (unk session)");

					iap2->Ack = hdr->SequenceNumber;
					_send_ack(iap2);
				}
	
				pool_free(&_input_pool, &buf->Node);
			}
		}
		else 
		{
			_verbose(VERBOSE_ERROR, "rx packet discarded (parse error)");
			pool_free(&_input_pool, &buf->Node);
		}
	}

	exos_dispatcher_add(context, dispatcher, EXOS_TIMEOUT_NEVER);
}


static bool _loop(iap2_context_t *iap2)
{
	iap2_transport_t *t = iap2->Transport;
	iap2_link_params_t *link = &t->LinkParams;
	ASSERT(link->RetransmitTimeout != 0, KERNEL_ERROR_KERNEL_PANIC);
	ASSERT(link->CumulativeAckTimeout != 0, KERNEL_ERROR_KERNEL_PANIC);

	// NOTE: init link params, the ones not provided by transport
	link->MaxRcvPacketLength = IAP2_BUFFER_SIZE;	// our max input buffer
	link->MaxNumOutstandingPackets = IAP2_BUFFERS;	// our max input buffers

	exos_dispatcher_context_create(&iap2->DispatcherContext);
	
	dispatcher_t rx_dispatcher;
	exos_dispatcher_create(&rx_dispatcher, _input_fifo.Event, _rx_callback, iap2);
	exos_dispatcher_add(&iap2->DispatcherContext, &rx_dispatcher, EXOS_TIMEOUT_NEVER);

	dispatcher_t sync_dispatcher;
	exos_dispatcher_create(&sync_dispatcher, &iap2->SyncEvent, _sync_callback, iap2);
	exos_dispatcher_add(&iap2->DispatcherContext, &sync_dispatcher, 500);

	while(!_service_exit)
	{
		// NOTE: timeout provides loop exit when theres no activity
		exos_dispatch(&iap2->DispatcherContext, 1000);
	}

	return true;
}


static bool _transport_switch_role(iap2_transport_t *t)
{
	bool done = false;
	const iap2_transport_driver_t *driver = t->Driver;
	if (driver->SwitchRole != NULL)
		done = driver->SwitchRole(t);
	return done;
} 


static void *_service(void *arg)
{
	iap2_transport_t *t = (iap2_transport_t *)arg;

	iap2_context_t iap2 = (iap2_context_t) { .Transport = t,
		.Seq = 0x2b}; // FIXME: randomize seq number
	exos_event_create(&iap2.SyncEvent, EXOS_EVENTF_AUTORESET);
	exos_fifo_create(&iap2.SyncFifo, &iap2.SyncEvent);
	
	// TODO: session setup according application needs, this is for testing purposes only
	for(unsigned i = 0; i < IAP2_MAX_SESSIONS; i++)
		iap2.Sessions[i] = (iap2_link_session1_t) { .Id = 10 + i, 
			.Type = (i == 0) ? IAP2_SESSION_TYPE_CONTROL : IAP2_SESSION_TYPE_EXTERNAL_ACCESSORY, 
			.Ver = 1 };

	exos_thread_sleep(1000);
	bool _role_switched = false;

	_verbose(VERBOSE_COMMENT, "service starting (%s)...", t->Id);

	if (_initialize(t))
	{
		_verbose(VERBOSE_DEBUG, "initialization procedure succeeded!");

		// NOTE: when we are NOT running in a hid host transport, this does nothing
		bool done = _transport_switch_role(t);
		if (done)
		{
			// TODO: notify the app that we, the 12 monkeys, did it
			_verbose(VERBOSE_COMMENT, "role-switch done...");
			_service_exit = true;
			_role_switched = true;
		}

		if (!_service_exit)
		{
			while(_loop(&iap2))
			{
				if (_service_exit) break;
				_verbose(VERBOSE_ERROR, "reset!");
				kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);	// re-entry kills dispatcher context!
			}
		}

//		iap_close_all();	// TODO! <<<<<<<<<<<<<<<<<<<<<
	}
	else
	{
		_verbose(VERBOSE_ERROR, "initialization procedure failed!");
	}

	_service_run = false;
	_verbose(VERBOSE_COMMENT, "service exiting (%s)...", t->Id);
	return NULL;
}
