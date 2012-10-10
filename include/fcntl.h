#ifndef __posix_fcntl_h
#define __posix_fcntl_h

#include <stdio.h>
#include <sys/types.h>

enum
{
	F_DUPFD = 1,
	F_DUPFD_CLOEXEC,
	F_GETFD, F_SETFD,
	F_GETFL, F_SETFL,
	F_GETLK, F_SETLK, F_SETLKW,
	F_GETOWN, F_SETOWN,
};

enum
{
	FD_CLOEXEC = 1,
};

enum
{
	F_RDLCK = 1,
	F_UNLCK,
	F_WRLCK,
};

enum
{
	O_CREAT = (1<<0),
	O_EXCL = (1<<1),
	O_NOCTTY = (1<<2),
	O_TRUNC = (1<<3),
	O_TTY_INIT = 0,

	O_APPEND = (1<<4),
	O_NONBLOCK = (1<<5),
	O_SYNC = (1<<6),
	
	O_EXEC = (1<<8),
	O_RDONLY = (1<<9),
	O_RDWR = (1<<10),
	O_SEARCH = (1<<11),
	O_WRONLY = (1<<12),

	O_CLOEXEC = (1<<13),
	O_DIRECTORY = (1<<14),
	O_NOFOLLOW = (1<<15),
};

struct flock
{
	short l_type;
	short l_whence;
	off_t l_start;
	off_t l_len;
//	pid_t l_pid;
};

int creat(const char *path, mode_t mode);
int fcntl(int fildes, int cmd, ...);
int open(const char *path, int oflag, ...);
int openat(int fd, const char *path, int oflag, ...);

#endif // __posix_fcntl_h


