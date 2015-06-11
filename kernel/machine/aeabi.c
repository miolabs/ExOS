#include <kernel/thread.h>
#include <kernel/panic.h>

#pragma GCC optimize(2)

unsigned char *__aeabi_read_tp(void)
{
	EXOS_THREAD *thread = __running_thread;
#ifdef DEBUG
	if (thread->LocalStorage == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	return thread->LocalStorage - 8;	// NOTE: glibc skips 8 bytes for DTV pointer (64bit)
}

