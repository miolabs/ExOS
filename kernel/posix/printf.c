#include <stdio.h>
#include <stdarg.h>
#include "posix.h"
#include <kernel/machine/hal.h>

int printf(const char *restrict format, ...)
{
	return -1;
}

int sprintf(char *restrict s, const char *restrict format, ...)
{
	va_list args;
	va_start(args, format);
	return vsprintf(s, format, args);
}

int vsprintf(char *restrict s, const char *restrict format, va_list args)
{
	int done = 0;
	while(1)
	{
		char c = *format++;
		if (c == '\0') break;
		
		if (c != '%')
		{
			s[done++] = c;
			continue;
		}

		c = *format++;
		if (c == '\0') break;

		int trailing_zeros = 1;
		while(c == '0')
		{
			trailing_zeros++;
			c = *format++;
		}
		if (c == '\0') break;

		int size = 4;	// FIXME: support signed/unsigned
		switch(c)
		{
			case 'l':
				size = 4;
				c = *format++;
				break;
			// TODO
		}

		if (c == '\0') break;

		switch(c)
		{
			case 's':
				done += __str_copy(s + done, va_arg(args, char *), -1);
				break;
			case 'd':
				done += __int32_declz(s + done, va_arg(args, int), trailing_zeros); 
				break;
			case 'x':
				switch (size)
				{
					case 4:
						done += __uint32_hexl(s + done, va_arg(args, int));
						break;
					// TODO
				}
				break;
			// TODO
		}
	}
	s[done] = '\0';
	return done;
}
