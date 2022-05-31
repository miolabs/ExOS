#ifndef EXOS_POSIX_POSIX_H
#define EXOS_POSIX_POSIX_H

#include <errno.h>

#include <kernel/io.h>
#include <kernel/mutex.h>

#ifndef POSIX_MAX_FILE_DESCRIPTORS
#define POSIX_MAX_FILE_DESCRIPTORS 8
#endif

typedef struct
{
	EXOS_MUTEX Mutex;
	EXOS_IO_ENTRY *Descriptors[POSIX_MAX_FILE_DESCRIPTORS];
} EXOS_POSIX_CONTEXT;

int posix_add_file_descriptor(EXOS_IO_ENTRY *io);
EXOS_IO_ENTRY *posix_get_file_descriptor(int fd);
EXOS_IO_ENTRY *posix_remove_file_descriptor(int fd);

int posix_set_error(posix_err_t error);

#endif // EXOS_POSIX_POSIX_H


