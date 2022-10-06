#include "iap2.h"
#include "../iap_fid.h"
#include "../cp20.h"
//#include "iap_comm.h"
#include <support/misc/pools.h>
#include <kernel/dispatch.h>
#include <kernel/verbose.h>
#include <kernel/panic.h>
#include <string.h> 

#ifdef IAP2_DEBUG
#define _verbose(level, ...) verbose(level, "iAP2-core", __VA_ARGS__)
#else
#define _verbose(level, ...) { /* nothing */ }
#endif


#define THREAD_STACK 2048
static exos_thread_t _thread;
static unsigned char _stack[THREAD_STACK] __attribute__((aligned(16)));
static void *_service(void *arg);
static volatile bool _service_run = false;
static bool _service_exit = false;

#define IAP2_BUFFERS 5
#define IAP2_BUFFER_SIZE 4096
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
	apple_cp20_initialize();

//	iap_comm_initialize();
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
	// TODO: support several instances
	ASSERT(_service_run == false, KERNEL_ERROR_KERNEL_PANIC);
	ASSERT(t != NULL && t->Driver != NULL, KERNEL_ERROR_NULL_POINTER);

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
	_service_run = false;

	_verbose(VERBOSE_DEBUG, "stopped!");
}

static bool _send(iap2_transport_t *t, const unsigned char *data, unsigned length)
{
	const iap2_transport_driver_t *driver = t->Driver;
	ASSERT(driver != NULL && driver->Send != NULL, KERNEL_ERROR_NULL_POINTER);
	return driver->Send(t, data, length);
}

void iap2_input(iap2_transport_t *t, const unsigned char *packet, unsigned packet_length)
{
	// FIXME: allow transport to allocate the buffer prior to filling it (zero copy)
	iap2_buffer_t *buf = (iap2_buffer_t *)pool_allocate(&_input_pool);
	if (buf != NULL)
	{
		ASSERT(packet_length < sizeof(buf->Buffer), KERNEL_ERROR_KERNEL_PANIC);
		memcpy(buf->Buffer, packet, packet_length);
		buf->Length = packet_length;

		exos_fifo_queue(&_input_fifo, &buf->Node);
	}
	else _verbose(VERBOSE_ERROR, "cannot allocate input buffer");
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
			.SequenceNumber = iap2->Seq++, .AckNumber = iap2->Ack };

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

	_verbose(VERBOSE_DEBUG, "send packet [sess #%d] ctrl=$%02x, seq=$%02x", 
		hdr->SessionId, hdr->Control, hdr->SequenceNumber);

	return _send(iap2->Transport, buf->Buffer, buf->Length);
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
		_verbose(VERBOSE_DEBUG, "[sync] rx [+%d] %d bytes!", input->Payload, input->Length);

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
		
		pool_free(&_output_pool, &buf->Node);	// FIXME: remove when no longer needed!	
	}
	else _verbose(VERBOSE_ERROR, "[sync] cannot allocate buffer!");

	exos_dispatcher_add(context, dispatcher, EXOS_TIMEOUT_NEVER);
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
		
			iap2->Ack = hdr->SequenceNumber;

			if (hdr->SessionId == 0)
			{
				buf->Payload = payload_offset;
				buf->Length = payload_len;
				exos_fifo_queue(&iap2->SyncFifo, &buf->Node);
			}
			else
			{
				// TODO
				_verbose(VERBOSE_ERROR, "rx packet discarded (unk session)");
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

	// init params
	link->MaxRcvPacketLength = IAP2_BUFFER_SIZE;	// our max input buffer
	link->MaxNumOutstandingPackets = IAP2_BUFFERS;	// our max input buffers

	dispatcher_context_t context;
	exos_dispatcher_context_create(&context);
	
	dispatcher_t rx_dispatcher;
	exos_dispatcher_create(&rx_dispatcher, _input_fifo.Event, _rx_callback, iap2);
	exos_dispatcher_add(&context, &rx_dispatcher, EXOS_TIMEOUT_NEVER);

	dispatcher_t sync_dispatcher;
	exos_dispatcher_create(&sync_dispatcher, &iap2->SyncEvent, _sync_callback, iap2);
	exos_dispatcher_add(&context, &sync_dispatcher, 500);

	while(!_service_exit)
	{
		// NOTE: timeout provides loop exit when theres no activity
		exos_dispatch(&context, 1000);
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
		_verbose(VERBOSE_DEBUG, "Initialization procedure succeded!");

		// NOTE: when we are NOT running in a hid host transport, this does nothing
		bool done = _transport_switch_role(t);
		if (done)
		{
			// TODO: notify the app that we, the 12 monkeys, did it
			_verbose(VERBOSE_COMMENT, "role-swith done...");
			_service_exit = true;
			_role_switched = true;
		}

		if (!_service_exit)
		{
			while(_loop(&iap2))
			{
				if (_service_exit) break;
				_verbose(VERBOSE_ERROR, "reset!");
			}
		}

//		iap_close_all();
	}
	else
	{
		_verbose(VERBOSE_ERROR, "Initialization procedure failed!");
	}

	_verbose(VERBOSE_COMMENT, "service exiting (%s)...", t->Id);
}
