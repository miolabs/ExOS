#ifndef EXOS_FIFO_H
#define EXOS_FIFO_H

#include <kernel/event.h>

typedef struct
{
#ifdef DEBUG
	unsigned Count;
#endif
	list_t Items;
	event_t *Event;
} fifo_t;

void exos_fifo_create(fifo_t *fifo, event_t *event);
node_t *exos_fifo_dequeue(fifo_t *fifo);
node_t *exos_fifo_wait(fifo_t *fifo, unsigned long timeout);
void exos_fifo_queue(fifo_t *fifo, node_t *node);

#endif // EXOS_FIFO_H
