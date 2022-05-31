
#include <kernel/panic.h>

#undef assert
#undef __assert

#ifdef NDEBUG
#define	assert(e)	((void)0)
#else

//#define __assert(e, file, line)  ((void)printf ("%s:%u: failed assertion `%s'\n", file, line, e), abort())

#define __assert(e, file, line)  ( kernel_panic( KERNEL_ERROR_UNKNOWN))

#define assert(e)  \
    ((void) ((e) ? 0 : __assert (#e, __FILE__, __LINE__)))

#endif /* NDEBUG */
