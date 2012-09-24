#include "startup.h"
#include "memory.h"
#include "thread.h"
#include "timer.h"
#include "machine/hal.h"

void main();

static EXOS_THREAD _main_thread;
static unsigned char _main_stack[MAIN_THREAD_STACK] __attribute__((aligned(16)));

void __kernel_start()
{
	__machine_init();
	__mem_init();
	__thread_init();
	__timer_init();

	// create the main thread
	exos_thread_create(&_main_thread, MAIN_THREAD_PRI, _main_stack, MAIN_THREAD_STACK, 
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
