#include "buffers.h"
#include <kernel/fifo.h>

#ifndef OHCI_MAX_BUFFERS
#define OHCI_MAX_BUFFERS 10	// at least MAX_PIPES + 1
#endif
#ifndef OHCI_MAX_PIPES
#define OHCI_MAX_PIPES 8
#endif

static OHCI_STD _buffers[OHCI_MAX_BUFFERS] __usb;
static OHCI_SED _pipes[OHCI_MAX_PIPES] __usb;

static fifo_t _free_buffers;
static fifo_t _free_pipes;

void ohci_buffers_initialize()
{
	exos_fifo_create(&_free_buffers, NULL);
	for(int i = 0; i < OHCI_MAX_BUFFERS; i++)
	{
		OHCI_STD *std = &_buffers[i];
		exos_fifo_queue(&_free_buffers, (node_t *)std);
	}

	exos_fifo_create(&_free_pipes, NULL);
	for(int i = 0; i < OHCI_MAX_PIPES; i++)
	{
		OHCI_SED *pipe = &_pipes[i];
		exos_fifo_queue(&_free_pipes, (node_t *)pipe);
	}
}

OHCI_STD *ohci_buffers_alloc_std()
{
	OHCI_STD *std = (OHCI_STD *)exos_fifo_dequeue(&_free_buffers);
	if (std != NULL)
	{
		std->Request = NULL;
        std->Status = OHCI_STD_STA_EMPTY;
	}
	return std;
}

void ohci_buffers_release_std(OHCI_STD *std)
{
	exos_fifo_queue(&_free_buffers, (node_t *)std);
}

OHCI_SED *ohci_buffers_alloc_sed()
{
	OHCI_SED *sed = (OHCI_SED *)exos_fifo_dequeue(&_free_pipes);
	return sed;
}

void ohci_buffers_release_sed(OHCI_SED *sed)
{
	exos_fifo_queue(&_free_pipes, (node_t *)sed);
}


