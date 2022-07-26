#ifndef EXOS_POSIX_POSIX_H
#define EXOS_POSIX_POSIX_H

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <kernel/io.h>
#include <kernel/mutex.h>

#ifndef POSIX_MAX_FILE_DESCRIPTORS
#define POSIX_MAX_FILE_DESCRIPTORS 8
#endif

struct __stdio_FILE
{
	io_entry_t io;
	mutex_t mutex;
};

int posix_add_file_descriptor(io_entry_t *io);
io_entry_t *posix_get_file_descriptor(int fd);
io_entry_t *posix_remove_file_descriptor(int fd);

int posix_set_error(posix_err_t error);

io_entry_t *posix_get_io_from_file(FILE *file);
	
#endif // EXOS_POSIX_POSIX_H


