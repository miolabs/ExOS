#ifndef __posix_stdio_h
#define __posix_stdio_h

#include <stddef.h>
#include <stdarg.h>

//int fprintf(FILE *restrict stream, const char *restrict format, ...);

// these are implemented in gcc library
int printf(const char *restrict format, ...);
int snprintf(char *restrict s, size_t n,
       const char *restrict format, ...);
int sprintf(char *restrict s, const char *restrict format, ...);
int vsprintf(char *restrict s, const char *restrict format, va_list args);

#endif // __posix_stdio_h


