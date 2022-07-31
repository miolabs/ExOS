#ifndef EXOS_IOBUFFER_H
#define EXOS_IOBUFFER_H

#include "event.h"

typedef struct
{
	unsigned char *Buffer;
	unsigned short Size;
	volatile unsigned short ProduceIndex;
	volatile unsigned short ConsumeIndex;
	event_t *NotEmptyEvent;
	event_t *NotFullEvent;
#ifdef EXOS_OLD
    event_t *EmptyEvent;
#endif
} io_buffer_t;

void exos_io_buffer_create(io_buffer_t *iobuf, unsigned char *buffer, unsigned short size, event_t *not_empty, event_t *not_full);
void exos_io_buffer_clear(io_buffer_t *iobuf);
unsigned exos_io_buffer_write(io_buffer_t *iobuf, const unsigned char *buffer, unsigned length);
unsigned exos_io_buffer_read(io_buffer_t *iobuf, unsigned char *buffer, unsigned length);


#ifdef EXOS_OLD
#define EXOS_IO_BUFFER stream_buffer_t

int exos_io_buffer_avail(EXOS_IO_BUFFER *iobuf);
int exos_io_buffer_discard(EXOS_IO_BUFFER *iobuf, unsigned length);
int exos_io_buffer_peek(EXOS_IO_BUFFER *iobuf, unsigned offset, void **pptr);
#endif 

#endif // EXOS_IOBUFFER_H
