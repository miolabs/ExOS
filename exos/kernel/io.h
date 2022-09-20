#ifndef EXOS_IO_H
#define EXOS_IO_H

#include <kernel/list.h>
#include <kernel/tree.h>
#include <kernel/event.h>
#include <stdbool.h>

typedef struct io_driver_t io_driver_t;

typedef enum
{
	IOF_NONE = 0,
	IOF_WRITE = 1<<4,
	IOF_CREATE = 1<<5,
	IOF_TRUNCATE = 1<<6,
	IOF_APPEND = 1<<7,
	IO_MODE_CREATE_NEW = (IOF_CREATE | IOF_WRITE | IOF_TRUNCATE),
	IO_MODE_READ_ONLY = 0,
} io_flags_t;

typedef struct
{
	node_t Node;
	const io_driver_t *Driver;
	void *DriverContext;
	unsigned Port;
	event_t InputEvent;
	event_t OutputEvent;
	unsigned long Timeout;
} io_entry_t;

typedef enum
{
	IO_OK = 0,
	IO_ERROR_UNKNOWN,
	IO_ERROR_NOT_IMPLEMENTED,
	IO_ERROR_NOT_ENOUGH_MEMORY,
	IO_ERROR_TIMEOUT,
	IO_ERROR_SEEK_MISALIGNMENT,
	IO_ERROR_DEVICE_NOT_FOUND,
	IO_ERROR_DEVICE_NOT_MOUNTED,
	IO_ERROR_VOLUME_ALREADY_MOUNTED,
	IO_ERROR_VOLUME_IN_USE,
	IO_ERROR_NOT_FORMATTED,
	IO_ERROR_INVALID_LOCK,
	IO_ERROR_VOLUME_NOT_FOUND,
	IO_ERROR_DIRECTORY_NOT_FOUND,
	IO_ERROR_FILE_NOT_FOUND,
	IO_ERROR_ALREADY_LOCKED,
	IO_ERROR_NOT_A_DIRECTORY,
	IO_ERROR_NOT_A_FILE,
	IO_ERROR_VOLUME_IS_FULL,
	IO_ERROR_READ_ERROR,
	IO_ERROR_WRITE_ERROR,
	IO_ERROR_SEEK_ERROR,
	IO_ERROR_IO_ERROR,
	IO_ERROR_NO_MORE_ENTRIES,
	IO_ERROR_ENUM_SIZE
} io_error_t;

typedef struct io_driver_t
{
	io_error_t (*Open)(io_entry_t *io, const char *path, io_flags_t flags);
	void (*Close)(io_entry_t *io);
	int (*Read)(io_entry_t *io, unsigned char *buffer, unsigned length);
	int (*Write)(io_entry_t *io, const unsigned char *buffer, unsigned length);
	int (*Sync)(io_entry_t *io);
} io_driver_t;

typedef struct
{
	exos_tree_node_t TreeNode;
	const io_driver_t *Driver;
	void *DriverContext;
	unsigned Port;
} io_tree_device_t;


#ifdef EXOS_OLD
#define EXOS_IO_ENTRY io_entry_t
#define EXOS_IO_DRIVER io_driver_t
#endif

void __io_initialize();
void exos_io_add_device(io_tree_device_t *device, const char *name, const io_driver_t *driver, void *driver_context);

io_error_t exos_io_open_path(io_entry_t *io, const char *path, io_flags_t flags);

void exos_io_create(io_entry_t *io, const io_driver_t *driver, void *driver_context, unsigned port) __deprecated;
io_error_t exos_io_open(io_entry_t *io, io_flags_t flags) __deprecated;
void exos_io_close(io_entry_t *io);
void exos_io_set_timeout(io_entry_t *io, unsigned long timeout);

int exos_io_read(io_entry_t *io, void *buffer, unsigned long length);
int exos_io_write(io_entry_t *io, const void *buffer, unsigned long length);
int exos_io_sync(io_entry_t *io);


#endif // EXOS_IO_H
