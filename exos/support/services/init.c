#include "init.h"
#include <kernel/machine/hal.h>
#include <kernel/panic.h>

extern int __ctors_start__, __ctors_end__;

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




