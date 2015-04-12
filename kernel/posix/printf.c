#include <stdio.h>
#include <stdarg.h>
#include "posix.h"
#include <kernel/datetime.h>
#include <kernel/machine/hal.h>
#include <support/services/debug.h>

__weak int printf(const char *restrict format, ...)
{
	va_list args;
	va_start(args, format);
	return debug_vprintf(format, args);
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

		int trailing_zeros = 0;
		char trailing_char = ' ';
		while(c >= '0' && c <= '9')
		{
			if (trailing_zeros == 0 && c == '0') trailing_char = '0';
			trailing_zeros = (trailing_zeros * 10) + (c - '0');
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
			//TODO: are more sizes possible?
		}

		if (c == '\0') break;

		switch(c)
		{
			case 's':
				done += __str_copy(s + done, va_arg(args, char *), -1);
				break;
			case 'c':
				s[done++] = (char)va_arg(args, int);
				break;
			case 'd':
				switch (size)
				{
					case 4:
						done += __int32_declz(s + done, va_arg(args, int), trailing_zeros, trailing_char); 
						break;
					//TODO: are more sizes possible?
				}
				break;
			case 'x':
				switch (size)
				{
					case 4:
						done += __uint32_hexlz(s + done, va_arg(args, int), trailing_zeros, trailing_char);
						break;
					//TODO: are more sizes possible?
				}
				break;
			case 'D':	//FIXME: non-standard!
				done += exos_datetime_print(s + done, va_arg(args, EXOS_DATETIME *));
				break;
		}
	}
	s[done] = '\0';
	return done;
}
