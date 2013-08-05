#ifndef EXOS_PANIC_H
#define EXOS_PANIC_H

#include <kernel/types.h>

typedef enum
{
	KERNEL_ERROR_UNKNOWN = 0,
	KERNEL_ERROR_NULL_POINTER,
	KERNEL_ERROR_STACK_OVERFLOW,
    KERNEL_ERROR_STACK_CORRUPTED,
    KERNEL_ERROR_STACK_INSUFFICIENT,
    KERNEL_ERROR_WRONG_NODE,
    KERNEL_ERROR_LIST_CORRUPTED,
	KERNEL_ERROR_LIST_ALREADY_CONTAINS_NODE,
    KERNEL_ERROR_THREAD_NOT_READY,
    KERNEL_ERROR_THREAD_NOT_WAITING,
	KERNEL_ERROR_CROSS_THREAD_OPERATION,
	KERNEL_ERROR_NO_SIGNALS_AVAILABLE,
	KERNEL_ERROR_MUTEX_IN_USE,
	KERNEL_ERROR_TIMER_NOT_FOUND,
	KERNEL_ERROR_TIMER_ALREADY_IN_USE,
    KERNEL_ERROR_EVENT_NOT_FOUND,
	KERNEL_ERROR_EVENT_WRONG_STATE,
	KERNEL_ERROR_IO_TYPE_MISMATCH,
	KERNEL_ERROR_MEMORY_CORRUPT,
	KERNEL_ERROR_THREAD_POOL_CORRUPTED,
	KERNEL_ERROR_ALREADY_IN_USE,
	KERNEL_ERROR_NO_HARDWARE_RESOURCES,
} KERNEL_ERROR;

void kernel_panic(KERNEL_ERROR error);
void __kernel_panic();

#endif // EXOS_PANIC_H

