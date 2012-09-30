#include "io.h"

void exos_io_create(EXOS_IO_ENTRY *io, EXOS_IO_TYPE type, const EXOS_IO_DRIVER *driver, EXOS_IO_FLAGS flags)
{
	*io = (EXOS_IO_ENTRY) {
#ifdef DEBUG
		.Node = (EXOS_NODE) { .Type = EXOS_NODE_IO_ENTRY },
#endif
		.Type = type, .Driver = driver, .Flags = flags };

	exos_event_create(&io->InputEvent, EXOS_EVENT_AUTO_RESET);
}

