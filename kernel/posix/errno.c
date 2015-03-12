//#include <errno.h>

int __errno;

#ifdef __ARM_EABI__

const int __aeabi_EDOM = 1;
const int __aeabi_EILSEQ = 2;
const int __aeabi_ERANGE = 3;

volatile int *__aeabi_errno_addr(void)
{
	return &__errno;
}

#endif