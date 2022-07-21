#include "verbose.h"
#include "panic.h"
#include <stdio.h>

#ifndef VERBOSE_LEVEL
#ifdef DEBUG
#define VERBOSE_LEVEL VERBOSE_DEBUG
#else
#define VERBOSE_LEVEL VERBOSE_QUIET
#endif
#endif

static verbose_t _current_level = VERBOSE_LEVEL;

void verbose_set_level(verbose_t level)
{
	_current_level = level;
}

int verbose(verbose_t level, const char *source, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	return vverbose(level, source, format, args);
}

int vverbose(verbose_t level, const char *source, const char *format, va_list args)
{
	ASSERT(source != nullptr, KERNEL_ERROR_NULL_POINTER);
	if (level >= _current_level)
	{
		printf("\n%s: ", source);
		return vprintf(format, args);
	}
	else
	{
		return 0;
	}
}



