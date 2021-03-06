#include "posix.h"
#include <errno.h>
#include <kernel/startup.h>
#include <kernel/thread_pool.h>

EXOS_POSIX_CONTEXT __main_context;
EXOS_THREAD_POOL __posix_thread_pool;

void __posix_init()
{
	exos_mutex_create(&__main_context.Mutex);
	exos_thread_pool_create(&__posix_thread_pool);

	//TODO: fill default descriptors and other context data

	EXOS_THREAD *thread = __running_thread;
	thread->ThreadContext = &__main_context;
}

int posix_add_file_descriptor(EXOS_IO_ENTRY *io)
{
	EXOS_POSIX_CONTEXT *context = (EXOS_POSIX_CONTEXT *)__running_thread->ThreadContext;
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

EXOS_IO_ENTRY *posix_get_file_descriptor(int fd)
{
	EXOS_POSIX_CONTEXT *context = (EXOS_POSIX_CONTEXT *)__running_thread->ThreadContext;
	if (context == NULL) return NULL;

	return (fd < POSIX_MAX_FILE_DESCRIPTORS) ? 
		(EXOS_IO_ENTRY *)context->Descriptors[fd] : NULL;	
}

EXOS_IO_ENTRY *posix_remove_file_descriptor(int fd)
{
	EXOS_POSIX_CONTEXT *context = (EXOS_POSIX_CONTEXT *)__running_thread->ThreadContext;
	if (context == NULL) return NULL;

	exos_mutex_lock(&context->Mutex);
	EXOS_IO_ENTRY *old = (fd < POSIX_MAX_FILE_DESCRIPTORS) ? 
		(EXOS_IO_ENTRY *)context->Descriptors[fd] : NULL;
	context->Descriptors[fd] = NULL;
    exos_mutex_unlock(&context->Mutex);
	return old;
}

int inline posix_set_error(posix_err_t error)
{
	errno = error;
	return -1;
}

