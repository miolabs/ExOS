#include "fifo.h"
#include "syscall.h"
#include "signal.h"
#include "panic.h"

void exos_fifo_create(fifo_t *fifo, event_t *event)
{
#ifdef DEBUG
	fifo->Count = 0;
#endif
	list_initialize(&fifo->Items);
	fifo->Event = event;
}

static int _dequeue(unsigned long *args)
{
	fifo_t *fifo = (fifo_t *)args[0];
	node_t **pnode = (node_t **)args[1];
	
	*pnode = list_rem_head(&fifo->Items);
#ifdef DEBUG
	if (*pnode != NULL)
	{
		if (fifo->Count == 0) kernel_panic(KERNEL_ERROR_LIST_CORRUPTED);
		fifo->Count--;
	}
	else
	{
		if (fifo->Count != 0) kernel_panic(KERNEL_ERROR_LIST_CORRUPTED);
	}
#endif
	if (fifo->Event != NULL)
	{
		if (LIST_ISEMPTY(&fifo->Items))
			exos_event_reset(fifo->Event);
	}
	return (*pnode != NULL);
}

node_t *exos_fifo_dequeue(fifo_t *fifo)
{
	node_t *node;
	__kernel_do(_dequeue, fifo, &node);
	return node;
}

node_t *exos_fifo_wait(fifo_t *fifo, unsigned long timeout)
{
#ifdef DEBUG
	if (fifo == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	if (fifo->Event != NULL && 
		!exos_event_wait(fifo->Event, timeout))
		return NULL;

	node_t *node;
	__kernel_do(_dequeue, fifo, &node);
#ifdef DEBUG
	if (node == NULL) kernel_panic(KERNEL_ERROR_UNKNOWN);
#endif
	return node;
}

static int _queue(unsigned long *args)
{
	fifo_t *fifo = (fifo_t *)args[0];
	node_t *node = (node_t *)args[1];

#ifdef DEBUG
	if (list_find_node(&fifo->Items, node))
		kernel_panic(KERNEL_ERROR_LIST_ALREADY_CONTAINS_NODE);
	fifo->Count++;
#endif
	list_add_tail(&fifo->Items, node);
   	if (fifo->Event != NULL)
	{
		exos_event_set(fifo->Event);
	}
	return 0;
}

void exos_fifo_queue(fifo_t *fifo, node_t *node)
{
	__kernel_do(_queue, fifo, node);
}

