#include "iap2.h"
#include "../iap_fid.h"
#include "../cp20.h"
//#include "iap_comm.h"
#include <kernel/fifo.h>
#include <kernel/verbose.h>
#include <kernel/panic.h>

#ifdef IAP_DEBUG
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

#define IAP2_MAX_PENDING_REQUESTS 2
static iap_request_t _requests[IAP2_MAX_PENDING_REQUESTS];
static EXOS_FIFO _free_requests_fifo;
static mutex_t _busy_requests_lock;
static list_t _busy_requests_list;

#define IAP2_MAX_INCOMING_COMMANDS 3
static iap_cmd_node_t _incoming_cmds[IAP2_MAX_INCOMING_COMMANDS];
static EXOS_FIFO _free_cmds_fifo;
static EXOS_FIFO _incoming_cmds_fifo;
static event_t _incoming_cmds_event;

void iap2_initialize()
{
	exos_fifo_create(&_free_requests_fifo, NULL);
	for(unsigned i = 0; i < IAP2_MAX_PENDING_REQUESTS; i++)
	{
		iap_request_t *req = &_requests[i];
		exos_event_create(&req->CompletedEvent, EXOS_EVENTF_AUTORESET);
		exos_fifo_queue(&_free_requests_fifo, &req->Node);
	}
	exos_mutex_create(&_busy_requests_lock);
	list_initialize(&_busy_requests_list);

	exos_fifo_create(&_free_cmds_fifo, NULL);
	for(unsigned i = 0; i < IAP2_MAX_INCOMING_COMMANDS; i++)
	{
		iap_cmd_node_t *cmd_node = &_incoming_cmds[i];
		exos_fifo_queue(&_free_cmds_fifo, &cmd_node->Node);
	}
	exos_event_create(&_incoming_cmds_event, EXOS_EVENTF_AUTORESET);
	exos_fifo_create(&_incoming_cmds_fifo, &_incoming_cmds_event);

	apple_cp20_initialize();

//   	iap_comm_initialize();
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

	_service_busy = true;
	exos_thread_create(&_thread, 5, _stack, THREAD_STACK, _service, t);
}

void iap2_stop()
{
	// TODO
	kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
}

static bool _send(iap2_transport_t *t, const unsigned char *data, unsigned length)
{
	const iap2_transport_driver_t *driver = t->Driver;
	ASSERT(driver != NULL && driver->Send != NULL, KERNEL_ERROR_NULL_POINTER);
	return driver->Send(t, data, length);
}

void iap2_parse(iap2_transport_t *t, const unsigned char *packet, unsigned packet_length)
{
	if (packet_length < sizeof(iap2_header_t))
	{
		_verbose(VERBOSE_ERROR, "packet header incomplete, length = %d", packet_length);
		return;
	}

	iap2_header_t *hdr = (iap2_header_t *)packet;
	unsigned short sop = IAP2SHTOH(hdr->Sop);
	unsigned short hdr_pkt_len = IAP2SHTOH(hdr->PacketLength);
	if (sop != 0xff5a || hdr_pkt_len != packet_length)
	{
		_verbose(VERBOSE_ERROR, "bad packet header", sop);
		return;
	}

	unsigned char checksum = hdr->Sop.High + hdr->Sop.Low + hdr->PacketLength.High + hdr->PacketLength.Low +
		hdr->Control + hdr->SequenceNumber + hdr->AckNumber + hdr->SessionId;
	if (checksum != hdr->Checksum)
	{
		_verbose(VERBOSE_ERROR, "packet header bad checksum ($%02x != $%02x)", hdr->Checksum, checksum);
		return;
	}

	// TODO

	_verbose(VERBOSE_DEBUG, "got packet (%s), %d bytes", t->Id, packet_length);
}





static bool _initialize(iap2_transport_t *t)
{
	const unsigned char _hello[] = { 0xff, 0x55, 0x02, 0x00, 0xee, 0x10 };

	bool done = false;
	for(unsigned i = 0; i < 30; i++)
	{
		if (!_send(t, _hello, sizeof(_hello)))
		{
			_verbose(VERBOSE_ERROR, "send hello failed!");
			break;
		}
		exos_thread_sleep(1000);
	}

	return done;
}

static void _slave_io()
{
}

static void *_service(void *arg)
{
	iap2_transport_t *t = (iap2_transport_t *)arg;

	exos_thread_sleep(1000);

	_verbose(VERBOSE_COMMENT, "Service starting (%s)...", t->Id);

	if (_initialize(t))
	{
		_verbose(VERBOSE_DEBUG, "Identify procedure succeded!");

		_slave_io();
//		iap_close_all();
	}
	else
	{
		_verbose(VERBOSE_ERROR, "Identify procedure failed!");
	}
	_verbose(VERBOSE_COMMENT, "Service exiting (%s)...", t->Id);
}
