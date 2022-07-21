#ifndef KERNEL_VERBOSE_H
#define KERNEL_VERBOSE_H

#include <stdarg.h>

typedef enum
{
	VERBOSE_DEBUG, VERBOSE_COMMENT, VERBOSE_ERROR, VERBOSE_CRITICAL, VERBOSE_QUIET
} verbose_t;

void verbose_set_level(verbose_t level);
int verbose(verbose_t level, const char *source, const char *format, ...);
int vverbose(verbose_t level, const char *source, const char *format, va_list args);

#endif // KERNEL_VERBOSE_H
