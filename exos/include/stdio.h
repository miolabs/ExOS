#ifndef __posix_stdio_h
#define __posix_stdio_h

#include <stddef.h>
#include <stdarg.h>

#define EOF (-1)

typedef struct __stdio_FILE FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

FILE *fopen(const char *pathname, const char *mode);
int fclose(FILE *stream);

int fprintf(FILE *stream, const char *format, ...);
int vfprintf(FILE *stream, const char *format, va_list args);
int fgetc(FILE *stream);
int fputc(int, FILE *stream);
int fputs(const char *s, FILE *stream);
size_t fread(void *buf, size_t el_size, size_t n_el, FILE *stream);
size_t fwrite(void *buf, size_t el_size, size_t n_el, FILE *stream);

int printf(const char *format, ...);
int vprintf(const char *format, va_list args);
int puts(const char *s);
int snprintf(char *s, size_t n, const char *format, ...);
int sprintf(char *s, const char *format, ...);
int vsprintf(char *s, const char *format, va_list args);

#endif // __posix_stdio_h


