#include <fcntl.h>
#include "posix.h"
#include <comm/comm.h>


int creat(const char *path, mode_t mode)
{
	return -1;
}

int fcntl(int fildes, int cmd, ...)
{
	return -1;
}

int open(const char *path, int oflag, ...)
{
	// NOT: varargs are currently ignored
	

	return -1;
}

int openat(int fd, const char *path, int oflag, ...)
{
	return -1;
}



