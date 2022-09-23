#ifndef EXOS_PORT_H
#define EXOS_PORT_H

#include <kernel/fifo.h>

typedef struct
{
	node_t Node;
	fifo_t Fifo;
	event_t Event;
	const char *Name;
} EXOS_PORT;

typedef struct
{
	node_t Node;
	EXOS_PORT *ReplyPort;
} EXOS_MESSAGE;

void __port_init();

int exos_port_create(EXOS_PORT *port, const char *name);
EXOS_PORT *exos_port_find(const char *name);
void exos_port_remove(const char *name);
EXOS_MESSAGE *exos_port_get_message(EXOS_PORT *port);
EXOS_MESSAGE *exos_port_wait_message(EXOS_PORT *port, int timeout);
void exos_port_send_message(EXOS_PORT *port, EXOS_MESSAGE *msg);
void exos_port_reply_message(EXOS_MESSAGE *msg);

#endif // EXOS_PORT_H

