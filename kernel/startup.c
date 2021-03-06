#include "startup.h"
#include "memory.h"
#include "thread.h"
#include "timer.h"
#include "io.h"
#include "port.h"
#include "machine/hal.h"
#include <modules/services/services.h>

#ifndef MAIN_THREAD_PRI
#define MAIN_THREAD_PRI 0
#endif

void main();

EXOS_THREAD _main_thread;
static unsigned char _main_stack[MAIN_THREAD_STACK] __attribute__((aligned(16)));

void __kernel_start()
{
	__machine_init();
	__mem_init();
	__port_init();
	__thread_init();
	__timer_init();
	__io_initialize();
	__posix_init();
	__services_init();

	// create the main thread
	exos_thread_create(&_main_thread, MAIN_THREAD_PRI, 
		_main_stack, MAIN_THREAD_STACK, NULL, 
		(EXOS_THREAD_FUNC)main, NULL);

	__machine_idle();
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

