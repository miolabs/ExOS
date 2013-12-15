#ifndef __posix_string_h
#define __posix_string_h

#include <stddef.h>

void *memset(void *s, int c, size_t n);

void *memcpy(void *restrict s1, const void *restrict s2, size_t n);

size_t strlen ( const char * str );

char *strerror(int errnum);

#endif // __posix_string_h
