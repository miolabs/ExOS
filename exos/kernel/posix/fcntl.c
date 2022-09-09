#include <fcntl.h>
#include "posix.h"

#include <kernel/memory.h> 
#include <kernel/io.h>
#include <stdarg.h>

int creat(const char *path, mode_t mode)
{
	return -1;
}

static int _set_fl(io_entry_t *io, int oflag)
{
	// FIXME
//	EXOS_IO_FLAGS flags = oflag & O_NONBLOCK ? EXOS_IOF_NONE : EXOS_IOF_WAIT;
//	exos_io_set_flags(io, flags);
	return 0;
}

int fcntl(int fd, int cmd, ...)
{
	va_list args;
	io_entry_t *io = posix_get_file_descriptor(fd);
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
	io_entry_t *io = (io_entry_t *)exos_mem_alloc(sizeof(io_entry_t), EXOS_MEMF_CLEAR);
	if (io == NULL) 
		return posix_set_error(ENOMEM);
	
	//EXOS_IO_FLAGS flags = oflag & O_NONBLOCK ? EXOS_IOF_NONE : EXOS_IOF_WAIT;
	io_error_t res = exos_io_open_path(io, path, IOF_NONE);	// TODO: parse flags
	if (res == IO_OK)
	{
		int fd = posix_add_file_descriptor((io_entry_t *)io);
		if (fd >= 0)
			return fd;
		
		exos_io_close(io);
	}
	exos_mem_free(io);
	return -1;
}

int openat(int fd, const char *path, int oflag, ...)
{
	return -1;
}



