#ifndef HANDLER_DEVICE_H
#define HANDLER_DEVICE_H

#include <kernel/io.h>
#include <kernel/iobuffer.h>

typedef struct
{
	io_tree_device_t Device;
	io_entry_t *Entry;
	io_buffer_t *Input;	// for read()
	io_buffer_t *Output; // for write()
	event_t OpenEvent;
} handler_context_t;

void handler_create_context(handler_context_t *hc, const char *name, io_buffer_t *input, io_buffer_t *output);

#endif // HANDLER_DEVICE_H


