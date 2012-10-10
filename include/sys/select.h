#ifndef __posix_sys_select_h
#define __posix_sys_select_h

#include <time.h>
#include <signal.h>

#define FD_SETSIZE 32

typedef struct
{
	unsigned mask;
} fd_set;

#define FD_CLR(index, fd) (fd)->mask &= ~(1 << (index))
#define FD_ISSET(index, fd) ((fd)->mask & (1 << (index)))
#define FD_SET(index, fd) (fd)->mask |= (1 << (index))
#define FD_ZERO(fd) (fd)->mask = (0)

int pselect(int, fd_set *restrict, fd_set *restrict, fd_set *restrict,
	const struct timespec *restrict, const sigset_t *restrict);
int select(int, fd_set *restrict, fd_set *restrict, fd_set *restrict,
	struct timespec *restrict);

#endif // __posix_sys_select_h
