#include "iap2.h"
#include "../iap_fid.h"
#include "../cp20.h"
//#include "iap_comm.h"
#include <kernel/fifo.h>
#include <support/misc/pools.h>
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
static bool _service_busy = false;
static bool _service_exit = false;

#define IAP2_BUFFERS 5
#define IAP2_BUFFER_SIZE 4096
typedef struct 
{ 
	node_t Node;
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
	pool_create(&_input_pool, (node_t *)_input, sizeof(iap2_buffer_t), IAP2_BUFFERS);
	exos_event_create(&_input_event, EXOS_EVENTF_AUTORESET);
	exos_fifo_create(&_input_fifo, &_input_event);

	pool_create(&_output_pool, (node_t *)_output, sizeof(iap2_buffer_t), IAP2_BUFFERS);

	apple_cp20_initialize();

//	iap_comm_initialize();
}

bool iap2_transport_create(iap2_transport_t *t, const char *id, const iap2_transport_driver_t *driver)
{
	t->Id = id;
	t->Driver = driver;
	return true;
}

bool iap2_start(iap2_transport_t *t)
{
	ASSERT(!_service_busy, KERNEL_ERROR_KERNEL_PANIC);
	ASSERT(t != NULL && t->Driver != NULL, KERNEL_ERROR_NULL_POINTER);

	t->Transaction = 1;

	_verbose(VERBOSE_DEBUG, "starting thread...");
	_service_busy = true;
	_service_exit = false;
	exos_thread_create(&_thread, 5, _stack, THREAD_STACK, _service, t);
}

void iap2_stop()
{
	_verbose(VERBOSE_DEBUG, "stopping thread...");
	_service_exit = true;
	exos_thread_join(&_thread);
	_verbose(VERBOSE_DEBUG, "stopped!");
	_service_busy = false;
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

static bool _parse_buffer(iap2_buffer_t *buf, iap2_header_t **pheader, void **ppayload)
{
	if (buf->Length < sizeof(iap2_header_t))
	{
		_verbose(VERBOSE_ERROR, "packet header incomplete, length = %d", buf->Length);
		return false;
	}

	iap2_header_t *hdr = (iap2_header_t *)buf->Buffer;
	unsigned short sop = IAP2SHTOH(hdr->Sop);
	unsigned short hdr_pkt_len = IAP2SHTOH(hdr->PacketLength);
	if (sop != 0xff5a || hdr_pkt_len > buf->Length)
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

	// TODO

	return false;
}

static unsigned _init_header(iap2_context_t *iap2, iap2_header_t *hdr, 
	unsigned char control, unsigned char sess_id, unsigned payload)
{
	hdr->Sop = HTOIAP2S(0xff5a);
	unsigned packet_length = sizeof(iap2_header_t) + payload + 1;	// NOTE: includes payload chksum
	hdr->PacketLength = HTOIAP2S(packet_length);
	hdr->Control = control;
	hdr->SequenceNumber = iap2->Seq;
	hdr->AckNumber = iap2->Ack;
	hdr->SessionId = sess_id;
	hdr->Checksum = _checksum((unsigned char *)hdr, sizeof(iap2_header_t) - 1);
	return sizeof(iap2_header_t);
}

static bool _send_syn(iap2_context_t *iap2, iap2_link_sync_payload1_t *sync, unsigned payload)
{
	iap2_buffer_t *buf = (iap2_buffer_t *)pool_allocate(&_output_pool);
	if (buf != NULL)
	{
		iap2_header_t *hdr = (iap2_header_t *)buf->Buffer;
		unsigned offset = _init_header(iap2, hdr,
			0x80, 0, payload);
		
		memcpy(buf->Buffer + offset, sync, payload);
		buf->Buffer[offset + payload] = _checksum(buf->Buffer + offset, payload);
		buf->Length = offset + payload + 1;

		_verbose(VERBOSE_DEBUG, "send SYN packet $%02x, length=$%x",
			iap2->Seq, buf->Length);

		_send(iap2->Transport, buf->Buffer, buf->Length);
		
		pool_free(&_output_pool, &buf->Node);
		return true;	
	}
	else
	{
		_verbose(VERBOSE_ERROR, "cannot allocate output buffer");
	}
	return false;
} 

static unsigned _init_link_payload(iap2_context_t *iap2, iap2_link_sync_payload1_t *sync)
{
	sync->LinkVersion = 0x01;
	sync->MaxNumOutstandingPackets = IAP2_BUFFERS;
	sync->MaxRcvPacketLength = HTOIAP2S(IAP2_BUFFER_SIZE);
	sync->RetxTimeout = HTOIAP2S(0x040B);	// FIXME
	sync->CumulativeAckTimeout = HTOIAP2S(0x0017);	// FIXME
	sync->MaxRetx = 3;
	sync->MaxCumulativeAcks = 3;
	unsigned offset = sizeof(iap2_link_sync_payload1_t);
	for(unsigned i = 0; i < IAP2_MAX_SESSIONS; i++)
	{
		sync->Sessions[i] = iap2->Sessions[i];
		offset += sizeof(iap2_link_session1_t);
	}
	return offset;
}

static bool _synchronize(iap2_context_t *iap2)
{
	bool done = false;

	iap2_link_sync_payload1_t sync;
	unsigned payload = _init_link_payload(iap2, &sync);
	// TODO?

	bool send_sync = true;
	while(!_service_exit)
	{
		if (send_sync)
		{
			_send_syn(iap2, &sync, payload);
		}

		iap2_buffer_t *buf = (iap2_buffer_t *)_wait_buffer(iap2->Transport, 1000);	// FIXME: configure timeout
		if (buf != NULL)
		{
			iap2_header_t *header;
			void *payload;
			if (_parse_buffer(buf, &header, &payload))
			{
				_verbose(VERBOSE_DEBUG, "[sync] got packet");
			}
			else _verbose(VERBOSE_ERROR, "[sync] packet discarded");

			_free_input_buffer(buf);
		}
		else break;
	}

	return done;
}

static void _slave_io()
{
	while(!_service_exit)
	{
		exos_thread_sleep(1000);
	}
}

static void *_service(void *arg)
{
	iap2_transport_t *t = (iap2_transport_t *)arg;

	iap2_context_t iap = (iap2_context_t) { .Transport = t,
		.Seq = 0x2b}; // FIXME: randomize seq number

	// TODO: session setup according application needs, this is for testing purposes only
	for(unsigned i = 0; i < IAP2_MAX_SESSIONS; i++)
		iap.Sessions[i] = (iap2_link_session1_t) { .Id = 10 + i, 
			.Type = (i == 0) ? IAP2_SESSION_TYPE_CONTROL : IAP2_SESSION_TYPE_EXTERNAL_ACCESSORY, 
			.Ver = 1 };

	exos_thread_sleep(1000);

	_verbose(VERBOSE_COMMENT, "Service starting (%s)...", t->Id);

	if (_initialize(t))
	{
		_verbose(VERBOSE_DEBUG, "Initialization procedure succeded!");

		while(!_synchronize(&iap))
		{
			_verbose(VERBOSE_ERROR, "Synchronization failed!");
			if (_service_exit) break;
		}

		_slave_io();
//		iap_close_all();
	}
	else
	{
		_verbose(VERBOSE_ERROR, "Identify procedure failed!");
	}
	_verbose(VERBOSE_COMMENT, "Service exiting (%s)...", t->Id);
}
