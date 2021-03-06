#include "iap_comm.h"
#include "iap_core.h"
#include <comm/comm.h>
#include <kernel/tree.h>
#include <stdio.h>
#ifdef IAP_DEBUG
#include <support/services/debug.h>
#endif

static int _open(COMM_IO_ENTRY *io);
static void _close(COMM_IO_ENTRY *io);
static int _get_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value);
static int _set_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value);
static int _read(COMM_IO_ENTRY *io, unsigned char *buffer, unsigned long length);
static int _write(COMM_IO_ENTRY *io, const unsigned char *buffer, unsigned long length);

static const COMM_DRIVER _comm_driver = {
	.Open = _open, .Close = _close,
    .GetAttr = _get_attr, .SetAttr = _set_attr, 
	.Read = _read, .Write = _write };
static COMM_DEVICE _comm_device = { .Driver = &_comm_driver };

static EXOS_LIST _managers;
static EXOS_MUTEX _managers_lock;
static int _protocol_count = 0;
#ifdef IAP_ACCESORY_EA_PROTOCOL_NAME
	static APPLE_IAP_PROTOCOL_MANAGER _manager1 = { .Name = IAP_ACCESORY_EA_PROTOCOL_NAME };
#endif

static EXOS_TREE_GROUP _iap_device_node = { .Name = "iap" };

void iap_comm_initialize()
{
	list_initialize(&_managers);
	exos_mutex_create(&_managers_lock);
	exos_tree_add_group(&_iap_device_node, "dev");

#ifdef IAP_ACCESORY_EA_PROTOCOL_NAME
	iap_comm_add_protocol(&_manager1);
#endif
}

void iap_comm_add_protocol(APPLE_IAP_PROTOCOL_MANAGER *iap)
{
	exos_mutex_lock(&_managers_lock);
	iap->ProtocolIndex = ++_protocol_count;
	iap->KernelDevice = (EXOS_TREE_DEVICE) { .Name = iap->Name, 
		.Device = &_comm_device, 
		.Unit = iap->ProtocolIndex - 1 };
	iap->Entry = NULL;
	iap->IOState = APPLE_IAP_IO_UNAVAILABLE;
	list_add_tail(&_managers, (EXOS_NODE *)iap);
	exos_mutex_unlock(&_managers_lock);

	comm_add_device(&iap->KernelDevice, "dev/iap");

#ifdef IAP_DEBUG
	debug_printf("Added iap protocol '%s'\r\n", iap->Name);
#endif
}

int iap_comm_get_protocol_count()
{
	return _protocol_count;
}

static APPLE_IAP_PROTOCOL_MANAGER *_find_session(unsigned short session_id)
{
	APPLE_IAP_PROTOCOL_MANAGER *found = NULL;
	exos_mutex_lock(&_managers_lock);
	FOREACH(node, &_managers)
	{
		APPLE_IAP_PROTOCOL_MANAGER *iap = (APPLE_IAP_PROTOCOL_MANAGER *)node;
		if (iap->IOState == APPLE_IAP_IO_OPENED &&
			iap->SessionID == session_id)
		{
			found = iap;
			break;
		}
	}
	exos_mutex_unlock(&_managers_lock);
	return found;
}

static APPLE_IAP_PROTOCOL_MANAGER *_find_protocol(unsigned short protocol_index)
{
	APPLE_IAP_PROTOCOL_MANAGER *found = NULL;
	exos_mutex_lock(&_managers_lock);
	FOREACH(node, &_managers)
	{
		APPLE_IAP_PROTOCOL_MANAGER *iap = (APPLE_IAP_PROTOCOL_MANAGER *)node;
		if (iap->ProtocolIndex == protocol_index)
		{
			found = iap;
			break;
		}
	}
	exos_mutex_unlock(&_managers_lock);
	return found;
}

APPLE_IAP_PROTOCOL_MANAGER *iap_comm_get_protocol(int index)
{
	return _find_protocol(index);
}

#ifdef DEBUG
static void _debug()
{
}
#endif

static int _open(COMM_IO_ENTRY *io)
{
	APPLE_IAP_PROTOCOL_MANAGER *iap = _find_protocol(io->Port + 1);
	if (iap != NULL &&
		iap->Entry == NULL &&
		iap->IOState == APPLE_IAP_IO_CLOSED)
	{
		exos_io_buffer_create(&iap->InputIOBuffer, iap->InputBuffer, APPLE_IAP_IO_BUFFER);
		iap->InputIOBuffer.NotEmptyEvent = &io->InputEvent;
		
		exos_event_create(&iap->InputEvent);
		exos_event_set(&iap->InputEvent);
		iap->InputIOBuffer.NotFullEvent = &iap->InputEvent;

//		exos_io_buffer_create(&iap->OutputIOBuffer, iap->OutputBuffer, APPLE_IAP_IO_BUFFER);
//		iap->OutputIOBuffer.NotFullEvent = &io->OutputEvent;
		
		iap->Entry = io;
		iap->IOState = APPLE_IAP_IO_OPENED;
		exos_event_set(&io->OutputEvent);

		exos_event_set(&io->SyncEvent);
		return 0;
	}
	return -1;
}

static void _close(COMM_IO_ENTRY *io)
{
	APPLE_IAP_PROTOCOL_MANAGER *iap = _find_protocol(io->Port + 1);
	if (iap != NULL &&
		iap->Entry == io &&
		(iap->IOState == APPLE_IAP_IO_OPENED || iap->IOState == APPLE_IAP_IO_DETACHED))
	{
		// TODO

		iap->IOState = (iap->IOState == APPLE_IAP_IO_DETACHED) ? APPLE_IAP_IO_UNAVAILABLE : APPLE_IAP_IO_CLOSED;
        iap->Entry = NULL;

        exos_event_set(&io->InputEvent);
	}
}

static int _get_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value)
{
	APPLE_IAP_PROTOCOL_MANAGER *iap = _find_protocol(io->Port + 1);
	if (iap != NULL)
	{
		if (attr == COMM_ATTR_SESSION_ID)
		{
			*(unsigned long *)value = (unsigned long)iap->SessionID; 
		}
	}
	return -1;
}

static int _set_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value)
{
	return -1;
}

static int _read(COMM_IO_ENTRY *io, unsigned char *buffer, unsigned long length)
{
	APPLE_IAP_PROTOCOL_MANAGER *iap = _find_protocol(io->Port + 1);
	if (iap != NULL &&
		(iap->IOState == APPLE_IAP_IO_OPENED || iap->IOState == APPLE_IAP_IO_DETACHED))
	{
		int done = exos_io_buffer_read(&iap->InputIOBuffer, buffer, length);

		if (iap->IOState == APPLE_IAP_IO_DETACHED && !io->InputEvent.State)
			_close(io);

		return done;
	}
	return -1;
}

static int _write(COMM_IO_ENTRY *io, const unsigned char *buffer, unsigned long length)
{
	unsigned char resp_buffer[8];
	APPLE_IAP_PROTOCOL_MANAGER *iap = _find_protocol(io->Port + 1);
	if (iap != NULL &&
		iap->IOState == APPLE_IAP_IO_OPENED)
	{
		int rem = length;
		while(rem > 0)
		{
			int fit = rem > (APPLE_IAP_IO_BUFFER - 2) ? (APPLE_IAP_IO_BUFFER - 2) : rem;
			IAP_CMD resp = (IAP_CMD) { .Length = fit + 2 };
			int offset = 0;
			iap->OutputBuffer[offset++] = iap->SessionID >> 8;
			iap->OutputBuffer[offset++] = iap->SessionID & 0xFF;
			for (int i = 0; i < fit; i++) iap->OutputBuffer[offset++] = *buffer++;
			IAP_CMD_STATUS status = iap_do_req3(IAP_CMD_ACCESORY_DATA_TRANSFER, iap->OutputBuffer, fit + 2, &resp, resp_buffer);
			if (status != IAP_OK)
				break;
			rem -= fit;
		}
		return rem == 0 ? length: -1;
	}
	return 1;
}


int iap_comm_write(unsigned short session_id, unsigned char *buffer, int length)
{
	for(int i = 0; i < 3; i++)
	{
		APPLE_IAP_PROTOCOL_MANAGER *iap = _find_session(session_id);
		if (iap != NULL &&
			iap->IOState == APPLE_IAP_IO_OPENED)
		{
			int rem = length;
			while(rem > 0)
			{
				if (exos_event_wait(iap->InputIOBuffer.NotFullEvent, 1000)) 
				{
#ifdef DEBUG
					_debug();
#endif
					break;	// timeout
				}
				int done = exos_io_buffer_write(&iap->InputIOBuffer, buffer, rem);
				buffer += done;
				rem -= done;
			}

#ifdef DEBUG
			iap->write_req_cnt++;
			iap->write_byte_cnt += length;
#endif
			return 1;
		}
		exos_thread_sleep(100);
	}
#ifdef DEBUG
	_debug();
#endif
	return 0;
}

static void _close_session(unsigned short session_id)
{
	APPLE_IAP_PROTOCOL_MANAGER *iap = _find_session(session_id);
	if (iap != NULL)
	{
		COMM_IO_ENTRY *io = iap->Entry;
		iap->IOState = (io != NULL) ? APPLE_IAP_IO_DETACHED : APPLE_IAP_IO_UNAVAILABLE;
	}
}

int iap_open_session(unsigned short session_id, unsigned short protocol_index)
{
	_close_session(session_id);
	
	APPLE_IAP_PROTOCOL_MANAGER *iap = _find_protocol(protocol_index);
	if (iap != NULL &&
		iap->IOState == APPLE_IAP_IO_UNAVAILABLE)	// FIXME: allow re-opening in CLOSING state
	{
		iap->SessionID = session_id;
		iap->IOState = APPLE_IAP_IO_CLOSED;

#ifdef DEBUG
		iap->write_req_cnt = iap->write_byte_cnt = 0;
#endif
		return 1;
	}
	return 0;
}

void iap_close_session(unsigned short session_id)
{
	_close_session(session_id);
}

void iap_close_all()
{
	exos_mutex_lock(&_managers_lock);
	FOREACH(node, &_managers)
	{
		APPLE_IAP_PROTOCOL_MANAGER *iap = (APPLE_IAP_PROTOCOL_MANAGER *)node;
        COMM_IO_ENTRY *io = iap->Entry;
		if (io != NULL)	comm_io_close(io);
		iap->IOState = APPLE_IAP_IO_UNAVAILABLE;
	}
	exos_mutex_unlock(&_managers_lock);
}


