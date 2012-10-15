#include <fcntl.h>
#include "posix.h"

#include <kernel/tree.h>
#include <kernel/memory.h> 
#include <comm/comm.h>
#include <stdarg.h>

int creat(const char *path, mode_t mode)
{
	return -1;
}

static int _set_fl(EXOS_IO_ENTRY *io, int oflag)
{
	EXOS_IO_FLAGS flags = oflag & O_NONBLOCK ? EXOS_IOF_NONE : EXOS_IOF_WAIT;
	exos_io_set_flags(io, flags);
	return 0;
}

int fcntl(int fd, int cmd, ...)
{
	va_list args;
	EXOS_IO_ENTRY *io = posix_get_file_descriptor(fd);
	if (io == NULL) return EBADF;	
	
	va_start(args, cmd);
	switch (cmd)
	{
		case F_SETFL:
			return _set_fl(io, va_arg(args, int));
	}
	return -1;
}

int open(const char *path, int oflag, ...)
{
	// NOT: varargs are currently ignored

	const char *dev_path = path;
	if (*dev_path == '/') dev_path++;
	EXOS_TREE_DEVICE *dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_node(NULL, &dev_path);
	if (dev_node == NULL)
		return posix_set_error(ENODEV);

	COMM_IO_ENTRY *io = (COMM_IO_ENTRY *)exos_mem_alloc(sizeof(COMM_IO_ENTRY), EXOS_MEMF_CLEAR);
	if (io == NULL) 
		return posix_set_error(ENOMEM);
	
	EXOS_IO_FLAGS flags = oflag & O_NONBLOCK ? EXOS_IOF_NONE : EXOS_IOF_WAIT;
	comm_io_create(io, dev_node->Device, dev_node->Port, flags);

	int error = comm_io_open(io, 0);
	if (error == 0)
	{
		int fd = posix_add_file_descriptor((EXOS_IO_ENTRY *)io);
		if (fd >= 0)
			return fd;
		
		comm_io_close(io);
	}
	exos_mem_free(io);
	return -1;
}

int openat(int fd, const char *path, int oflag, ...)
{
	return -1;
}



