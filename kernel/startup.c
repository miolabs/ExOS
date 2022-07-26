#include "startup.h"
#include "memory.h"
#include "thread.h"
#include "timer.h"
#include "io.h"
#include "port.h"
#include "machine/hal.h"
#include <modules/services/services.h>

#ifdef MAIN_THREAD_STACK
#warning "MAIN_THREAD_STACK is no longer used; use linker (process) stack definition for initial thread"
#endif

extern int main();

void __kernel_start()
{
	__machine_init();
	__thread_init();
	__mem_init();
	__port_init();
	__timer_init();
	__io_initialize();
	__posix_init();
	__services_init();

	main();

	__machine_reset();
}

__weak void __machine_init()
{
	// weak initializer does nothing
}

__weak void __machine_idle()
{
	while(1);
}

__weak void __services_init()
{
	// weak initializer does nothing
}

