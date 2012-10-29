#include <unistd.h>
#include "posix.h"
#include <kernel/thread.h>
#include <kernel/timer.h>
#include <kernel/memory.h>
#include <kernel/panic.h>
#include <comm/comm.h>
#include <net/net_io.h>

int close(int fd)
{
	EXOS_IO_ENTRY *io = posix_get_file_descriptor(fd);
	if (io == NULL) return posix_set_error(EBADF);
	switch(io->Type)
	{
		case EXOS_IO_COMM:
			comm_io_close((COMM_IO_ENTRY *)io);
			break;
		case EXOS_IO_SOCKET:
			net_io_close((NET_IO_ENTRY *)io);
			break;
		default:
			return posix_set_error(EBADF);
	}

#ifdef DEBUG
	EXOS_IO_ENTRY *io_old = posix_remove_file_descriptor(fd);
	if (io_old != io) kernel_panic(KERNEL_ERROR_LIST_CORRUPTED);
#else
	posix_remove_file_descriptor(fd);
#endif
	exos_mem_free(io);
	return 0;
}

ssize_t pread(int fd, void *buf, size_t nbyte, off_t offset)
{
	// TODO: call seek() (if stream) and then read()
	return -1;
}

ssize_t read(int fd, void *buf, size_t nbyte)
{
	EXOS_IO_ENTRY *io = posix_get_file_descriptor(fd);
	if (io == NULL) return posix_set_error(EBADF);
	return exos_io_read(io, buf, nbyte);
}

ssize_t pwrite(int fd, const void *buf, size_t nbyte, off_t offset)
{
	// TODO: call seek() (if stream) and then read()
	return -1;
}

ssize_t write(int fd, const void *buf, size_t nbyte)
{
	EXOS_IO_ENTRY *io = posix_get_file_descriptor(fd);
	if (io == NULL) return posix_set_error(EBADF);
	return exos_io_write(io, buf, nbyte);
}

unsigned sleep(unsigned seconds)
{
	exos_thread_sleep(seconds * EXOS_TICK_FREQ);
	return 0;
	// NOTE: should return remaining time 
	// but that case should not happen in ExOS
}
