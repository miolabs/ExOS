#include "fifo.h"
#include "syscall.h"
#include "signal.h"
#include "panic.h"

void exos_fifo_create(EXOS_FIFO *fifo)
{
	list_initialize(&fifo->Items);
	list_initialize(&fifo->Handles);
}

static int _dequeue(unsigned long *args)
{
	EXOS_FIFO *fifo = (EXOS_FIFO *)args[0];
	EXOS_NODE **pnode = (EXOS_NODE **)args[1];
	
	*pnode = list_rem_head(&fifo->Items);
	return (*pnode != NULL);
}

EXOS_NODE *exos_fifo_dequeue(EXOS_FIFO *fifo)
{
	EXOS_NODE *node;
	__kernel_do(_dequeue, fifo, &node);
	return node;
}

static int _dequeue_wait(unsigned long *args)
{
	EXOS_FIFO *fifo = (EXOS_FIFO *)args[0];
	EXOS_NODE **pnode = (EXOS_NODE **)args[1];
	EXOS_WAIT_HANDLE *handle = (EXOS_WAIT_HANDLE *)args[2];
	
	EXOS_NODE *node = list_rem_head(&fifo->Items);
	if (node != NULL)
	{
		*pnode = node;
		return 1;
	}
	__cond_add_wait_handle(&fifo->Handles, handle);
	*pnode = NULL;
	return 0;
}

EXOS_NODE *exos_fifo_wait(EXOS_FIFO *fifo, unsigned long timeout)
{
#ifdef DEBUG
	if (fifo == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	EXOS_NODE *node;
	EXOS_WAIT_HANDLE handle;
	while (!__kernel_do(_dequeue_wait, fifo, &node, &handle))
	{
		unsigned long mask = exos_signal_wait(1 << handle.Signal, timeout);
		if (mask == EXOS_SIGF_ABORT)
		{
			exos_cond_abort(&handle);
			return NULL;	// timeout
		}
	}
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
	return __cond_signal_all(&fifo->Handles);
}

void exos_fifo_queue(EXOS_FIFO *fifo, EXOS_NODE *node)
{
	__kernel_do(_queue, fifo, node);
}

