#include "fifo.h"
#include "syscall.h"
#include "signal.h"
#include "panic.h"

void exos_fifo_create(EXOS_FIFO *fifo, EXOS_EVENT *event)
{
	list_initialize(&fifo->Items);
	fifo->Event = event;
}

static int _dequeue(unsigned long *args)
{
	EXOS_FIFO *fifo = (EXOS_FIFO *)args[0];
	EXOS_NODE **pnode = (EXOS_NODE **)args[1];
	
	*pnode = list_rem_head(&fifo->Items);
	if (fifo->Event != NULL)
		__set_event(fifo->Event, !LIST_ISEMPTY(&fifo->Items));
	return (*pnode != NULL);
}

EXOS_NODE *exos_fifo_dequeue(EXOS_FIFO *fifo)
{
	EXOS_NODE *node;
	__kernel_do(_dequeue, fifo, &node);
	return node;
}

EXOS_NODE *exos_fifo_wait(EXOS_FIFO *fifo, unsigned long timeout)
{
#ifdef DEBUG
	if (fifo == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	if (fifo->Event != NULL && exos_event_wait(fifo->Event, timeout) < 0)
		return NULL;

	EXOS_NODE *node;
	__kernel_do(_dequeue, fifo, &node);
#ifdef DEBUG
	if (node == NULL) __kernel_panic();
#endif
	return node;
}

static int _queue(unsigned long *args)
{
	EXOS_FIFO *fifo = (EXOS_FIFO *)args[0];
	EXOS_NODE *node = (EXOS_NODE *)args[1];

	list_add_tail(&fifo->Items, node);
   	if (fifo->Event != NULL)
		__set_event(fifo->Event, 1);

	return 0;
}

void exos_fifo_queue(EXOS_FIFO *fifo, EXOS_NODE *node)
{
	__kernel_do(_queue, fifo, node);
}

