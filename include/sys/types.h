// ExOS posix layer
// reference:
// http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/pthread.h.html#tag_13_36

#ifndef __posix_types_h
#define __posix_types_h

#include <kernel/types.h>
#include <kernel/thread.h>

// [...]

typedef unsigned long mode_t; // TODO

// [...]

typedef unsigned long off_t; // TODO: signed?

// [...]

// WIP: struct types:
//pthread_attr_t
//pthread_barrier_t
//pthread_barrierattr_t
//pthread_cond_t
//pthread_condattr_t
//pthread_key_t
//pthread_mutex_t
//pthread_mutexattr_t
//pthread_once_t
//pthread_rwlock_t
//pthread_rwlockattr_t
//pthread_spinlock_t
typedef struct
{
	EXOS_THREAD exos_thread;
	// TODO: add posix state support
} pthread_t;

// WIP: struct types:
//trace_attr_t
//trace_event_id_t
//trace_event_set_t
//trace_id_t

// [...]

typedef unsigned long size_t;
typedef long ssize_t; 
typedef long long suseconds_t;
typedef unsigned long time_t;
typedef int timer_t;	// TODO


#endif // __posix_types_h
