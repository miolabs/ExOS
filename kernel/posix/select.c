#include <sys/select.h>
#include "posix.h"
#include <comm/comm.h>

int pselect(int nfds, fd_set *restrict readfds, 
	fd_set *restrict writefds, fd_set *restrict errorfds,
	const struct timespec *restrict tiemout, 
	const sigset_t *restrict sigmask)
{
	return -1;
}

int select(int nfds, fd_set *restrict readfds, 
	fd_set *restrict writefds, fd_set *restrict errorfds,
	struct timespec *restrict timeout)
{
	return -1;
}


