#ifndef EXOS_FIFO_H
#define EXOS_FIFO_H

#include <kernel/event.h>

typedef struct
{
#ifdef DEBUG
	int Count;
#endif
	list_t Items;
	event_t *Event;
} EXOS_FIFO;

void exos_fifo_create(EXOS_FIFO *fifo, event_t *event);
node_t *exos_fifo_dequeue(EXOS_FIFO *fifo);
node_t *exos_fifo_wait(EXOS_FIFO *fifo, unsigned long timeout);
void exos_fifo_queue(EXOS_FIFO *fifo, node_t *node);

#endif // EXOS_FIFO_H
