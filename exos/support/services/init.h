#ifndef EXOS_INIT_H
#define EXOS_INIT_H

#include <kernel/list.h>

typedef enum
{
	EXOS_INIT_HIGHEST_PRI = 50,
	EXOS_INIT_HW_DRIVER = 40,
	EXOS_INIT_IO_DRIVER = 30,
	EXOS_INIT_NET_DRIVER = 25,
	EXOS_INIT_FS_DRIVER = 20,
	EXOS_INIT_POSIX = 15,
	EXOS_INIT_SW_DRIVER = 10,
	EXOS_INIT_SERVICE = 0,
	EXOS_INIT_LOWEST_PRI = -128
} EXOS_INIT_PRI;

typedef void *EXOS_CTOR;

typedef struct
{
	node_t;
	void (*Func)();
} EXOS_INIT_NODE;

#ifdef DEBUG
#define EXOS_INITIALIZER(name, pri, func) \
	static EXOS_INIT_NODE name = { .Type = EXOS_NODE_INITIALIZER, .Priority = pri, .Func = func }; \
	static __used const EXOS_CTOR __attribute__((section(".ctors"))) name##_ctor = &name
#else
#define EXOS_INITIALIZER(name, pri, func) \
	static EXOS_INIT_NODE name = { .Priority = pri, .Func = func }; \
	static __used const EXOS_CTOR __attribute__((section(".ctors"))) name##_ctor = &name
#endif

#endif // EXOS_INIT_H

