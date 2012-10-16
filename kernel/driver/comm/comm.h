#ifndef COMM_COMM_H
#define COMM_COMM_H

#include <kernel/io.h>
#include <kernel/mutex.h>
#include <kernel/thread.h>

#ifndef COMM_DEVICE_THREAD_STACK
#define COMM_DEVICE_THREAD_STACK 512
#endif

typedef struct _COMM_DRIVER COMM_DRIVER;

typedef struct
{
	const COMM_DRIVER *Driver;
	unsigned PortCount;

//	unsigned long InputSignal;
//	EXOS_THREAD Thread;
//	unsigned char Stack[COMM_DEVICE_THREAD_STACK];
} COMM_DEVICE;

typedef struct 
{
	EXOS_IO_ENTRY;
	COMM_DEVICE *Device;
	unsigned Port;
	unsigned long Baudrate;
} COMM_IO_ENTRY;

struct _COMM_DRIVER
{
	int (*Open)(COMM_IO_ENTRY *io);
	int (*SetAttrs)(COMM_IO_ENTRY *io);
	void (*Close)(COMM_IO_ENTRY *io);
	int (*Read)(COMM_IO_ENTRY *io, unsigned char *buffer, unsigned long length);
	int (*Write)(COMM_IO_ENTRY *io, const unsigned char *buffer, unsigned long length);
};


void comm_initialize();
void comm_io_create(COMM_IO_ENTRY *io, COMM_DEVICE *device, unsigned port, EXOS_IO_FLAGS flags);
int comm_io_open(COMM_IO_ENTRY *io, int baudrate);
int comm_io_set_baudrate(COMM_IO_ENTRY *io, int baudrate);
void comm_io_close(COMM_IO_ENTRY *io);

#endif // COMM_COMM_H
