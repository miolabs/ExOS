#ifndef __posix_string_h
#define __posix_string_h

#include <stddef.h>

void *memcpy(void *restrict s1, const void *restrict s2, size_t n);

char *strerror(int errnum);

#endif // __posix_string_h
