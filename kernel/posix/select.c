#include <sys/select.h>
#include "posix.h"
#include <sys/select.h>
#include <kernel/io.h>

static int _wait(int nfds, 
		fd_set *readfds, fd_set *writefds, int timeout)
{
	EXOS_WAIT_HANDLE handles[nfds];

	unsigned long used = (readfds != NULL ? readfds->mask : 0) | 
		(writefds != NULL ? writefds->mask : 0);
	int count = 0;

	unsigned long wait_mask = 0;
	for(int fd = 0; fd < nfds; fd++)
	{
		unsigned long fd_mask = (1 << fd);
		if (used & fd_mask)
		{
			EXOS_IO_ENTRY *io = posix_get_file_descriptor(fd);
			if (io == NULL) return posix_set_error(EBADF);

			if (readfds->mask & fd_mask)
				exos_event_check(&io->InputEvent, &handles[fd], &wait_mask);
		}
	}
	if (wait_mask != 0)
		exos_signal_wait(wait_mask, timeout);
	
	for(int fd = 0; fd < nfds; fd++)
	{
		unsigned long fd_mask = (1 << fd);
		if (used & fd_mask)
		{
			EXOS_IO_ENTRY *io = posix_get_file_descriptor(fd);
			
			if (readfds->mask & fd_mask)
			{
				if (io->InputEvent.State) count++;
				else FD_CLR(fd, readfds);
				
				EXOS_WAIT_HANDLE *handle = &handles[fd];
				if (handle->State == EXOS_WAIT_PENDING)
					exos_cond_abort(handle);
			}
		}
	}

	return count;
}

static const unsigned long _nanos_per_tick = 1000000000L / EXOS_TICK_FREQ;
static const unsigned long _micros_per_tick = 1000000L / EXOS_TICK_FREQ;
 
int pselect(int nfds, 
	fd_set *restrict readfds, 
	fd_set *restrict writefds, 
	fd_set *restrict errorfds,
	const struct timespec *restrict timeout, 
	const sigset_t *restrict sigmask)
{
	// NOTE: sigmask is currently ignored
	
	if (timeout == NULL)
	{
		return _wait(nfds, readfds, writefds, EXOS_TIMEOUT_NEVER);
	}

	unsigned long timeout_ticks = (timeout->tv_sec * EXOS_TICK_FREQ) + 
		(timeout->tv_nsec / _nanos_per_tick);
	if (timeout_ticks == 0) timeout_ticks = 1; // FIXME: 0 means forever in exos
	
	return _wait(nfds, readfds, writefds, timeout_ticks);
}

int select(int nfds, 
	fd_set *restrict readfds, 
	fd_set *restrict writefds, 
	fd_set *restrict errorfds,
	struct timeval *restrict timeout)
{
	if (timeout == NULL)
	{
		return _wait(nfds, readfds, writefds, EXOS_TIMEOUT_NEVER);
	}

	unsigned long timeout_ticks = (timeout->tv_sec * EXOS_TICK_FREQ) + 
		(timeout->tv_usec / _micros_per_tick);
	if (timeout_ticks == 0) timeout_ticks = 1; // FIXME: 0 means forever in exos
	
	return _wait(nfds, readfds, writefds, timeout_ticks);
}


