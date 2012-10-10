#include <unistd.h>
#include "posix.h"
#include <kernel/thread.h>
#include <kernel/timer.h>

int close(int fd)
{
	return -1;
}

ssize_t pread(int fd, void *buf, size_t nbyte, off_t offset)
{
}

ssize_t read(int fd, void *buf, size_t nbyte)
{
}

ssize_t pwrite(int fd, const void *buf, size_t nbyte, off_t offset)
{
}

ssize_t write(int fd, const void *buf, size_t nbyte)
{
}

unsigned sleep(unsigned seconds)
{
	exos_thread_sleep(seconds * EXOS_TICK_FREQ);
	return 0;
	// NOTE: should return remaining time 
	// but that case should not happen in ExOS
}
