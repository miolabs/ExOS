#include "posix.h"
#include <errno.h>
#include <kernel/startup.h>
#include <kernel/thread_pool.h>
#include <kernel/panic.h>

typedef struct
{
	mutex_t Mutex;
	io_entry_t *Descriptors[POSIX_MAX_FILE_DESCRIPTORS];
} exos_posix_context_t;

exos_posix_context_t __main_context;
exos_thread_pool_t __posix_thread_pool;

FILE *stdin = nullptr;
FILE *stdout = nullptr;
FILE *stderr = nullptr;

void __posix_init()
{
	exos_mutex_create(&__main_context.Mutex);
	exos_thread_pool_create(&__posix_thread_pool);

	//TODO: fill default descriptors and other context data

	exos_thread_t *thread = __running_thread;
//	thread->ThreadContext = &__main_context;
//	TODO: implement using TLS EABI
}

static exos_posix_context_t *_get_posix_context()
{
//	TODO: implement using TLS EABI
	exos_posix_context_t *context = NULL; //(exos_posix_context_t *)__running_thread->ThreadContext;
	return context;
}

int posix_add_file_descriptor(io_entry_t *io)
{
	exos_posix_context_t *context = _get_posix_context();
	if (context == NULL) return ENOTSUP;

	exos_mutex_lock(&context->Mutex);
	int fd = EMFILE;
	for(int i = 0; i < POSIX_MAX_FILE_DESCRIPTORS; i++)
	{
		if (context->Descriptors[i] == NULL)
		{
			context->Descriptors[i] = io;
			fd = i;
			break;
		}
	}
	exos_mutex_unlock(&context->Mutex);
	return fd;
}

io_entry_t *posix_get_file_descriptor(int fd)
{
	exos_posix_context_t *context = _get_posix_context();
	if (context == NULL) return NULL;

	return (fd < POSIX_MAX_FILE_DESCRIPTORS) ? 
		(io_entry_t *)context->Descriptors[fd] : NULL;	
}

io_entry_t *posix_remove_file_descriptor(int fd)
{
	exos_posix_context_t *context = _get_posix_context();
	if (context == NULL) return NULL;

	exos_mutex_lock(&context->Mutex);
	io_entry_t *old = (fd < POSIX_MAX_FILE_DESCRIPTORS) ? 
		(io_entry_t *)context->Descriptors[fd] : NULL;
	context->Descriptors[fd] = NULL;
    exos_mutex_unlock(&context->Mutex);
	return old;
}

int inline posix_set_error(posix_err_t error)
{
	errno = error;
	return -1;
}

io_entry_t *posix_get_io_from_file(FILE *file)
{
	ASSERT(file != NULL, KERNEL_ERROR_NULL_POINTER);
	return &file->io;
}
