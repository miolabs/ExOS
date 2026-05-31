#include "init.h"
#include <kernel/machine/hal.h>
#include <kernel/panic.h>

// Boundaries of the initializer-pointer section (see EXOS_CTORS_SECTION in
// init.h). On ELF the linker script defines these; on Mach-O the linker
// synthesises section$start$/section$end$ symbols for any named section, which
// we bind to the same names via asm labels so the loop below is unchanged.
#if defined(__MACH__)
extern int __ctors_start__ __asm("section$start$__DATA$__exos_ctors");
extern int __ctors_end__ __asm("section$end$__DATA$__exos_ctors");
#else
extern int __ctors_start__, __ctors_end__;
#endif

static list_t _init_list;

static void _init_func();

void __services_init()
{
	list_initialize(&_init_list);

	void **ctor = (void **)&__ctors_start__;
	while(ctor < (void **)&__ctors_end__)
	{
		EXOS_INIT_NODE *init = *(EXOS_INIT_NODE **)ctor;
#ifdef DEBUG
		if (init->Type != EXOS_NODE_INITIALIZER)
			kernel_panic(KERNEL_ERROR_WRONG_NODE);
#endif
		list_enqueue(&_init_list, (node_t *)init);
		ctor++;
	}

	FOREACH(node, &_init_list)
	{
		EXOS_INIT_NODE *init = (EXOS_INIT_NODE *)node;
		if (init->Func == NULL)
			kernel_panic(KERNEL_ERROR_NULL_POINTER);
		
		init->Func();
	}
}




