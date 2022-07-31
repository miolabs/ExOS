#ifndef __posix_sys_select_h
#define __posix_sys_select_h

#include <time.h>
#include <sys/time.h>
#include <signal.h>

#define FD_SETSIZE 32

typedef struct
{
	unsigned mask;
} fd_set;

#define FD_CLR(index, fdset) (fdset)->mask &= ~(1 << (index))
#define FD_ISSET(index, fdset) ((fdset)->mask & (1 << (index)))
#define FD_SET(index, fdset) (fdset)->mask |= (1 << (index))
#define FD_ZERO(fdset) (fdset)->mask = (0)

int pselect(int, fd_set *restrict, fd_set *restrict, fd_set *restrict,
	const struct timespec *restrict, const sigset_t *restrict);
int select(int, fd_set *restrict, fd_set *restrict, fd_set *restrict,
	struct timeval *restrict);

#endif // __posix_sys_select_h
