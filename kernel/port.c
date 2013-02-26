#include "port.h"
#include <kernel/fifo.h>
#include <kernel/mutex.h>
#include <kernel/panic.h>
#include <kernel/machine/hal.h>

static EXOS_LIST _port_list;
static EXOS_MUTEX _port_mutex;

static EXOS_PORT *_find_port(const char *name);

void __port_init()
{
	list_initialize(&_port_list);
	exos_mutex_create(&_port_mutex);
}

int exos_port_create(EXOS_PORT *port, const char *name)
{
	port->Node = (EXOS_NODE) {
#ifdef DEBUG
		.Type = EXOS_NODE_PORT,
#endif
		};

	exos_event_create(&port->Event);
	exos_fifo_create(&port->Fifo, &port->Event);
	
	port->Name = name;
	int done = 1;
	if (name != NULL)
	{
		exos_mutex_lock(&_port_mutex);
		if (NULL == _find_port(name))
		{
			list_add_tail(&_port_list, (EXOS_NODE *)port);
		}
		else done = 0;
		exos_mutex_unlock(&_port_mutex);
	}
	return done;
}

static EXOS_PORT *_find_port(const char *name)
{
	FOREACH(node, &_port_list)
	{
		EXOS_PORT *port = (EXOS_PORT *)node;;
		if (__str_comp(name, port->Name) == 0) return port;
	}
	return NULL;
}

EXOS_PORT *exos_port_find(const char *name)
{
	if (name == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);

	exos_mutex_lock(&_port_mutex);
	EXOS_PORT *port = _find_port(name);
	exos_mutex_unlock(&_port_mutex);
}

void exos_port_remove(const char *name)
{
	if (name == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);

	exos_mutex_lock(&_port_mutex);
	EXOS_PORT *port = _find_port(name);
	if (port != NULL) list_remove(port);
	exos_mutex_unlock(&_port_mutex);
}

EXOS_MESSAGE *exos_port_get_message(EXOS_PORT *port, int timeout)
{
	EXOS_NODE *node = exos_fifo_wait(&port->Fifo, timeout);
	if (node != NULL)
	{
#ifdef DEBUG
		if (node->Type != EXOS_NODE_MESSAGE) kernel_panic(KERNEL_ERROR_WRONG_NODE);
#endif
		return (EXOS_MESSAGE *)node;
	}
	return NULL;
}

void exos_port_send_message(EXOS_PORT *port, EXOS_MESSAGE *msg)
{
	if (port == NULL || msg == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#ifdef DEBUG
	if (port->Node.Type != EXOS_NODE_PORT) kernel_panic(KERNEL_ERROR_WRONG_NODE);
	msg->Node.Type = EXOS_NODE_MESSAGE;
#endif
	exos_fifo_queue(&port->Fifo, (EXOS_NODE *)msg);
}

void exos_port_reply_message(EXOS_MESSAGE *msg)
{
	if (msg == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#ifdef DEBUG
	if (msg->Node.Type != EXOS_NODE_MESSAGE) kernel_panic(KERNEL_ERROR_WRONG_NODE);
#endif
	exos_port_send_message(msg->ReplyPort, msg);
}



