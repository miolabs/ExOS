#ifndef EXOS_IO_H
#define EXOS_IO_H

#include <kernel/event.h>

typedef struct _EXOS_IO_DRIVER EXOS_IO_DRIVER;

typedef enum
{
	EXOS_IO_COMM = 0,
	EXOS_IO_SOCKET,
	EXOS_IO_FILE,
} EXOS_IO_TYPE;

typedef enum
{
	EXOS_IOF_NONE = 0,
	EXOS_IOF_WAIT = (1<<0),
} EXOS_IO_FLAGS;

typedef struct 
{
	EXOS_NODE Node;
	EXOS_IO_TYPE Type;
	const EXOS_IO_DRIVER *Driver;
	EXOS_IO_FLAGS Flags;
	unsigned long Timeout;
   	EXOS_EVENT InputEvent;
   	EXOS_EVENT OutputEvent;
} EXOS_IO_ENTRY;

struct _EXOS_IO_DRIVER
{
	int (*Read)(EXOS_IO_ENTRY *io, void *buffer, unsigned long length);
	int (*Write)(EXOS_IO_ENTRY *io, const void *buffer, unsigned long length);
};

void __io_initialize();
void exos_io_create(EXOS_IO_ENTRY *io, EXOS_IO_TYPE type, const EXOS_IO_DRIVER *driver, EXOS_IO_FLAGS flags);
void exos_io_set_flags(EXOS_IO_ENTRY *io, EXOS_IO_FLAGS flags);
void exos_io_set_timeout(EXOS_IO_ENTRY *io, unsigned long timeout);
int exos_io_read(EXOS_IO_ENTRY *io, void *buffer, unsigned long length);
int exos_io_write(EXOS_IO_ENTRY *io, const void *buffer, unsigned long length);

typedef struct
{
	void *RcvBuffer;
	void *SndBuffer;
	unsigned short RcvBufferSize;
	unsigned short SndBufferSize;
} EXOS_IO_STREAM_BUFFERS;

typedef struct
{
	unsigned char *Buffer;
	unsigned short Size;
	volatile unsigned short ProduceIndex;
	volatile unsigned short ConsumeIndex;
	EXOS_EVENT *NotFullEvent;
	EXOS_EVENT *NotEmptyEvent;
} EXOS_IO_BUFFER;

void exos_io_buffer_create(EXOS_IO_BUFFER *iobuf, void *buffer, unsigned short size);
int exos_io_buffer_write(EXOS_IO_BUFFER *iobuf, void *buffer, unsigned short length);
int exos_io_buffer_read(EXOS_IO_BUFFER *iobuf, void *buffer, unsigned short length);
int exos_io_buffer_avail(EXOS_IO_BUFFER *iobuf);
int exos_io_buffer_discard(EXOS_IO_BUFFER *iobuf, unsigned short length);
int exos_io_buffer_peek(EXOS_IO_BUFFER *iobuf, unsigned short offset, void **pptr);

#endif // EXOS_IO_H
