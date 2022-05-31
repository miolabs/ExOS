#include <errno.h>

__thread int __errno_val;

#ifdef __ARM_EABI__

const int __aeabi_EDOM = 33;
const int __aeabi_EILSEQ = 47;
const int __aeabi_ERANGE = 34;

volatile int *__aeabi_errno_addr(void)
{
	return &__errno_val;
}

#endif