#include "startup.h"
#include "threads.h"
<<<<<<< HEAD
#include "machine/hal.h"

void main();

static EXOS_THREAD _main_thread;
static unsigned char _main_stack[MAIN_THREAD_STACK] __attribute__((aligned(16)));

void __kernel_start()
{
	__machine_init();
	//__mem_init();
	__threads_init();

	// create the main thread
	threads_create(&_main_thread, MAIN_THREAD_PRI, _main_stack, MAIN_THREAD_STACK, (THREAD_FUNC)main, NULL);

	__machine_idle();
=======

void main();

void __kernel_start()
{
	__machine_init();
	//mem_init();
	__threads_init();

	main();
>>>>>>> 5c314a936eb4b82d55df2ee2c11c7f12fc824acb
}

__weak void __machine_init()
{
	// weak initializer does nothing
}

<<<<<<< HEAD
__weak void __machine_idle()
{
	while(1);
}
=======

>>>>>>> 5c314a936eb4b82d55df2ee2c11c7f12fc824acb

