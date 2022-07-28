#include "port.h"
#include <kernel/fifo.h>
#include <kernel/mutex.h>
#include <kernel/panic.h>
#include <kernel/machine/hal.h>

static list_t _port_list;
static mutex_t _port_mutex;

static EXOS_PORT *_find_port(const char *name);

void __port_init()
{
	list_initialize(&_port_list);
	exos_mutex_create(&_port_mutex);
}

int exos_port_create(EXOS_PORT *port, const char *name)
{
	port->Node = (node_t) {	.Type = EXOS_NODE_PORT };

	exos_event_create(&port->Event, EXOS_EVENTF_AUTORESET);
	exos_fifo_create(&port->Fifo, &port->Event);
	
	port->Name = name;
	int done = 1;
	if (name != NULL)
	{
		exos_mutex_lock(&_port_mutex);
		if (NULL == _find_port(name))
		{
			list_add_tail(&_port_list, (node_t *)port);
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
	ASSERT(name != NULL, KERNEL_ERROR_NULL_POINTER);

	exos_mutex_lock(&_port_mutex);
	EXOS_PORT *port = _find_port(name);
	exos_mutex_unlock(&_port_mutex);
	return 0;
}

void exos_port_remove(const char *name)
{
	ASSERT(name != NULL, KERNEL_ERROR_NULL_POINTER);

	exos_mutex_lock(&_port_mutex);
	EXOS_PORT *port = _find_port(name);
	if (port != NULL) list_remove(&port->Node);
	exos_mutex_unlock(&_port_mutex);
}

EXOS_MESSAGE *exos_port_get_message(EXOS_PORT *port)
{
	node_t *node = exos_fifo_dequeue(&port->Fifo);
	if (node != NULL)
	{
		ASSERT(node->Type == EXOS_NODE_MESSAGE, KERNEL_ERROR_WRONG_NODE);
		return (EXOS_MESSAGE *)node;
	}
	return NULL;
}

EXOS_MESSAGE *exos_port_wait_message(EXOS_PORT *port, int timeout)
{
	node_t *node = exos_fifo_wait(&port->Fifo, timeout);
	if (node != NULL)
	{
		ASSERT(node->Type == EXOS_NODE_MESSAGE, KERNEL_ERROR_WRONG_NODE);
		return (EXOS_MESSAGE *)node;
	}
	return NULL;
}

void exos_port_send_message(EXOS_PORT *port, EXOS_MESSAGE *msg)
{
	ASSERT(port != NULL && msg == NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(port->Node.Type == EXOS_NODE_PORT, KERNEL_ERROR_WRONG_NODE);
	msg->Node.Type = EXOS_NODE_MESSAGE;
	exos_fifo_queue(&port->Fifo, (node_t *)msg);
}

void exos_port_reply_message(EXOS_MESSAGE *msg)
{
	ASSERT(msg != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(msg->Node.Type == EXOS_NODE_MESSAGE, KERNEL_ERROR_WRONG_NODE);
	exos_port_send_message(msg->ReplyPort, msg);
}



