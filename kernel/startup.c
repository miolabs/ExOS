#include "startup.h"
#include "threads.h"
#include "machine/hal.h"

void main();

void __kernel_start()
{
	__machine_init();
	//mem_init();
	__threads_init();

	main();
}

__weak void __machine_init()
{
	// weak initializer does nothing
}



