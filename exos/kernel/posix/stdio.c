#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "posix.h"
#include <kernel/mutex.h>
#include <kernel/io.h>
#include <kernel/memory.h>
#include <kernel/machine/hal.h>
#include <kernel/panic.h>
#include <support/services/init.h>

static bool _open(FILE *stream, const char *path, io_flags_t flags);
static int _vformat(unsigned (*vputs)(void *state, const char *s, unsigned length), const char *format, va_list args, void *state);

#ifdef STDOUT_STREAM

static void _register();
EXOS_INITIALIZER(_init, EXOS_INIT_POSIX, _register);

static void _register()
{
	static FILE _stdout;

	if (_open(&_stdout, STDOUT_STREAM, IOF_CREATE | IOF_WRITE | IOF_APPEND))
	{
#ifdef STDOUT_TIMEOUT
		io_set_timeout(&_stdout.io, STDOUT_TIMEOUT);
#endif
		stdout = &_stdout;
		stderr = &_stdout;	// FIXME
		stdin = &_stdout;	// FIXME
	}
	else
	{
		kernel_panic(KERNEL_ERROR_KERNEL_PANIC);
	}

}
#endif


static bool _open(FILE *stream, const char *path, io_flags_t flags)
{
	ASSERT(stream != nullptr, KERNEL_ERROR_NULL_POINTER);
	if (IO_OK == exos_io_open_path(&stream->io, path, flags))
	{
		exos_mutex_create(&stream->mutex);
		return true;
	}
	return false;
}

static io_flags_t _parse_fopen_mode(const char *mode)
{
	ASSERT(mode != nullptr, KERNEL_ERROR_NULL_POINTER);
	switch(mode[0])
	{
		case 'r':	return (mode[1] == '+') ? IOF_WRITE : IOF_NONE;
		case 'w':	return IOF_CREATE | IOF_TRUNCATE | IOF_WRITE;	// NOTE: we don't support write-only streams
		case 'a':	return IOF_CREATE | IOF_WRITE | IOF_APPEND;	// NOTE: we don't support write-only streams
	}
	return IOF_NONE;
}

FILE *fopen(const char *pathname, const char *mode)
{
	FILE *stream = malloc(sizeof(FILE));
	if (stream != NULL)
	{
		io_flags_t flags = _parse_fopen_mode(mode);

		if (!_open(stream, pathname, flags))
		{
			free(stream);
			stream = NULL;
		}
	}
	return stream;
}

int fclose(FILE *stream)
{
	ASSERT(stream != nullptr, KERNEL_ERROR_NULL_POINTER);
	exos_io_close(&stream->io);
	free(stream);
	return 0;
}

io_entry_t *__get_FILE_io(FILE *stream)
{
	ASSERT(stream != nullptr, KERNEL_ERROR_NULL_POINTER);
	return &stream->io;
}

int printf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	return vprintf(format, args);
}

int vprintf(const char *format, va_list args)
{
	if (stdout != nullptr)
		return vfprintf(stdout, format, args);
	return -1;
}

int fprintf(FILE *stream, const char *format, ...)
{
	if (stream != nullptr)
	{
		va_list args;
		va_start(args, format);
		return vfprintf(stream, format, args);
	}
	return -1;
}

static unsigned _io_puts(void *state, const char *src, unsigned length)
{
	io_entry_t *io = (io_entry_t *)state;
	for(unsigned i = 0; i < length; i++)
		if (src[i] == '\0')
		{
			length = i;
			break;
		}
	return exos_io_write(io, src, length);
}

int vfprintf(FILE *stream, const char *format, va_list args)
{
	ASSERT(stream != nullptr, KERNEL_ERROR_NULL_POINTER);
	exos_mutex_lock(&stream->mutex);
	unsigned done = _vformat(_io_puts, format, args, &stream->io);
	exos_mutex_unlock(&stream->mutex);
	return done;
}

int fgetc(FILE *stream)
{
	ASSERT(stream != nullptr, KERNEL_ERROR_NULL_POINTER);

	int c;
	exos_mutex_lock(&stream->mutex);
	int done = exos_io_read(&stream->io, &c, 1);
	exos_mutex_unlock(&stream->mutex);
	
	return (done < 0) ? done :
		((done == 0) ? EOF : c);
}

int fputc(int c, FILE *stream)
{
	ASSERT(stream != nullptr, KERNEL_ERROR_NULL_POINTER);
	exos_mutex_lock(&stream->mutex);
	int done = exos_io_write(&stream->io, &c, 1);
	exos_mutex_unlock(&stream->mutex);
	return done;
}

int puts(const char *s)
{
	if (stdout != nullptr)
		return fputs(s, stdout);
	else 
		return -1;
}

int fputs(const char *s, FILE *stream)
{
	ASSERT(stream != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(s != nullptr, KERNEL_ERROR_NULL_POINTER);
	size_t length = strlen(s);
	exos_mutex_lock(&stream->mutex);
	int done = exos_io_write(&stream->io, s, length);
	exos_mutex_unlock(&stream->mutex);
	return done;
}

size_t fread(void *buf, size_t el_size, size_t n_el, FILE *stream)
{
	ASSERT(stream != nullptr, KERNEL_ERROR_NULL_POINTER);
	size_t length = n_el * el_size;
	if (length == 0)
		return 0;

	exos_mutex_lock(&stream->mutex);
	int done = exos_io_read(&stream->io, buf, length);
	exos_mutex_unlock(&stream->mutex);
	return (done <= 0) ? done : (done / el_size);
}

size_t fwrite(void *buf, size_t el_size, size_t n_el, FILE *stream)
{
	ASSERT(stream != nullptr, KERNEL_ERROR_NULL_POINTER);
	size_t length = n_el * el_size;
	if (length == 0)
		return 0;

	exos_mutex_lock(&stream->mutex);
	int done = exos_io_write(&stream->io, buf, length);
	exos_mutex_unlock(&stream->mutex);
	return (done <= 0) ? done : (done / el_size);
}

int sprintf(char *s, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	return vsprintf(s, format, args);
}

static unsigned _mem_puts(void *state, const char *src, unsigned length)
{
	char **pptr = (char **)state;
	unsigned actual = __str_copy(*pptr, src, length);
	*pptr += actual;
	return actual;
}

int vsprintf(char *s, const char *format, va_list args)
{
	int length = _vformat(_mem_puts, format, args, &s);
	*s = '\0';
	return length;
}

static int _vformat(unsigned (*vputs)(void *state, const char *s, unsigned length), const char *format, va_list args, void *state)
{
	int length = 0;
	char buf[64];
	unsigned buf_length = 0;
	int done = 0;
	char c, pad = ' ';

	while(c = *format++, c != '\0')
	{
		if (c == '%')
		{
			if (buf_length != 0)
			{
				done = vputs(state, format - (buf_length + 1), buf_length);
				buf_length = 0;

				if (done < 0)
				{
					length = done;	// NOTE: report error
					break;
				}
				
				length += done;
				done = 0;
			}

			enum { fmt_int = 0, fmt_long = (1<<0), fmt_uppercase = (1<<1) } flags = fmt_int;
			unsigned int fmt_size = 0;

			if (*format == '0')
			{
				pad = '0';
				format++;
			}

			format += __decl_uint(format, &fmt_size);

			c = *format++;
			if (c == '\0') break;
			
			if (c == 'l' || c == 'L')
			{
				flags |= fmt_long;
				c = *format++;
				if (c == '\0') 
					break;
			}

			switch(c)
			{
				case 's':
					done = vputs(state, va_arg(args, char *), -1);
					break;
				case 'd':
					buf_length = (flags & fmt_long) ?
						__long_declz(buf, va_arg(args, long), fmt_size, pad) :
						__int_declz(buf, va_arg(args, int), fmt_size, pad);
					ASSERT(buf_length < sizeof(buf), KERNEL_ERROR_KERNEL_PANIC);
					done = vputs(state, buf, buf_length);
					break;
				case 'u':
					buf_length = (flags & fmt_long) ?
						__long_declz(buf, va_arg(args, unsigned long), fmt_size, pad) :
						__uint_declz(buf, va_arg(args, unsigned int), fmt_size, pad);
					ASSERT(buf_length < sizeof(buf), KERNEL_ERROR_KERNEL_PANIC);
					done = vputs(state, buf, buf_length);
					break;
				case 'X':
					flags |= fmt_uppercase;
				case 'x':
					buf_length = (flags & fmt_long) ?
						__ulong_hexlz(buf, va_arg(args, unsigned long), fmt_size, pad) :
						__uint_hexlz(buf, va_arg(args, unsigned int), fmt_size, pad);
					ASSERT(buf_length < sizeof(buf), KERNEL_ERROR_KERNEL_PANIC);
					done = vputs(state, buf, buf_length);
					break;
#ifdef STDIO_FORMAT_FLOAT
				case 'f':
					buf_length = __float_decl(buf, (float)va_arg(args, double), fmt_size ? fmt_size : 6);
					ASSERT(buf_length < sizeof(buf), KERNEL_ERROR_KERNEL_PANIC);
					done = vputs(state, buf, buf_length);
					break;
#endif
				case 'c':
					c = (char)va_arg(args, int);
				default:
					done = vputs(state, &c, 1);	// FIXME: buffer
					break;
			}
			
			if (done < 0)
			{
				length = done;	// NOTE: report error in length
				break;
			}

			length += done;
			buf_length = 0;
			done = 0;
		}
		else 
		{
			buf_length++;
		}
	}

	if (buf_length != 0 && length >= 0)
	{
		done = vputs(state, format - (buf_length + 1), buf_length);
		length = (done < 0) ? done : (length + done);
	}
	return length;
}



