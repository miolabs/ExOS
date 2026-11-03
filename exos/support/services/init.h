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

// The initializer pointers are gathered into one section whose boundaries the
// linker exposes as __ctors_start__/__ctors_end__ (see init.c).
//  * Bare-metal ELF (the firmware): a plain ".ctors"; the board's linker
//    script defines the boundary symbols.
//  * Hosted ELF (Linux): ".ctors" is NOT usable -- the GNU default linker
//    script folds it into .init_array and the C runtime would call our node
//    pointers as functions. Use an own section; for a C-identifier section name
//    GNU ld synthesises __start_<name>/__stop_<name> automatically.
//  * Mach-O (macOS): needs a "segment,section" pair; ld64 supplies the
//    boundaries via the magic section$start$/section$end$ names.
#if defined(__MACH__)
#define EXOS_CTORS_SECTION "__DATA,__exos_ctors"
#elif defined(__linux__)
#define EXOS_CTORS_SECTION "exos_ctors"
#else
#define EXOS_CTORS_SECTION ".ctors"
#endif

#define EXOS_INITIALIZER(name, pri, func) \
	static EXOS_INIT_NODE name = { .Type = EXOS_NODE_INITIALIZER, .Priority = pri, .Func = func }; \
	static __used const EXOS_CTOR __attribute__((section(EXOS_CTORS_SECTION))) name##_ctor = &name

#endif // EXOS_INIT_H

