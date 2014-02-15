#include "panic.h"

static KERNEL_ERROR __kernel_last_error;

void abort(void)
{
	kernel_panic( KERNEL_ERROR_UNKNOWN);
}


void kernel_panic(KERNEL_ERROR error)
{
	__kernel_last_error = error;
	__kernel_panic();
}

void __weak __kernel_panic()
{
	while(1);
}
