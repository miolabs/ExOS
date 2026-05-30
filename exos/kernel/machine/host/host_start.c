// Host process entry point.
//
// On a microcontroller the reset vector lands in startup code that ends by
// calling __kernel_start(), and the application's main() becomes the kernel's
// initial thread. On the host the C runtime owns the real main(), so this is
// where the process begins: bring the kernel up, which in turn runs the
// application (renamed to exos_app_main via -DEXOS_APP_MAIN) as its first
// thread. __kernel_start() never returns in normal operation.
//
// Compiled WITHOUT exos/include: only the kernel startup prototype is needed.

#include <kernel/startup.h>

int main(void)
{
	__kernel_start();
	return 0;
}
