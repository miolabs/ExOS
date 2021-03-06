#ifndef EXOS_FIFO_H
#define EXOS_FIFO_H

#include <kernel/event.h>

typedef struct
{
#ifdef DEBUG
	int Count;
#endif
	EXOS_LIST Items;
	EXOS_EVENT *Event;
} EXOS_FIFO;

void exos_fifo_create(EXOS_FIFO *fifo, EXOS_EVENT *event);
EXOS_NODE *exos_fifo_dequeue(EXOS_FIFO *fifo);
EXOS_NODE *exos_fifo_wait(EXOS_FIFO *fifo, unsigned long timeout);
void exos_fifo_queue(EXOS_FIFO *fifo, EXOS_NODE *node);

#endif // EXOS_FIFO_H
