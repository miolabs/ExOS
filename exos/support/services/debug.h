#ifndef DEBUG_H
#define DEBUG_H

#include <stdarg.h>

int debug_printf(const char *format, ...);
int debug_vprintf(const char *format, va_list args);
int debug_print(const char *buffer, int length);

#endif // DEBUG_H


