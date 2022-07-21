// ExOS posix layer
// reference:
// http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/pthread.h.html#tag_13_36

#ifndef __posix_types_h
#define __posix_types_h

#include <kernel/thread.h>
#include <kernel/mutex.h>

typedef unsigned long size_t;
typedef long ssize_t; 


typedef long long suseconds_t;
typedef unsigned long time_t;
typedef int timer_t;	// TODO: Used for timer ID returned by timer_create()


typedef unsigned long mode_t; // TODO
typedef unsigned long off_t; // TODO: signed?

//pthread_barrier_t
//pthread_barrierattr_t
typedef struct __pthread_cond_t pthread_cond_t;
typedef struct __pthread_condattr_t pthread_condattr_t;

//pthread_key_t

typedef struct
{
} pthread_mutexattr_t;

typedef struct
{
	mutex_t native_mutex;
} pthread_mutex_t;

extern const pthread_mutex_t __mutex_initializer;
#define PTHREAD_MUTEX_INITIALIZER { { { NULL, (EXOS_NODE *)-1, NULL }, NULL, 0 } }

//pthread_once_t
//pthread_rwlock_t
//pthread_rwlockattr_t
//pthread_spinlock_t

typedef EXOS_THREAD pthread_info_t;

typedef struct
{
	size_t stack_size;
	void *stack;
	int pri;
} pthread_attr_t;

typedef struct
{
	pthread_info_t *info;
} pthread_t;

//trace_attr_t
//trace_event_id_t
//trace_event_set_t
//trace_id_t


#endif // __posix_types_h
