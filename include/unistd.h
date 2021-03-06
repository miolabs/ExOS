// ExOS posix layer
// reference:
// http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/unistd.h.html#tag_13_80

#ifndef __posix_unistd_h
#define __posix_unistd_h

#include <sys/types.h>

int close(int fildes);

ssize_t pread(int fd, void *buf, size_t nbyte, off_t offset);
ssize_t read(int fd, void *buf, size_t nbyte);
ssize_t pwrite(int fd, const void *buf, size_t nbyte, off_t offset);
ssize_t write(int fd, const void *buf, size_t nbyte);

#define _POSIX_FSYNC

int fsync(int fd);

unsigned sleep(unsigned);


#endif // __posix_unistd_h

