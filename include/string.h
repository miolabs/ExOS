#ifndef __posix_string_h
#define __posix_string_h

#include <stddef.h>

void *memset(void *s, int c, size_t n);

void *memcpy(void *restrict s1, const void *restrict s2, size_t n);

int strcmp(const char *str1, const char *str2);
size_t strlen(const char *str);
char *strcpy(char *dest, const char *src);
int strncmp(const char *str1, const char *str2, size_t num);
char *strncpy(char *dest, const char *src, size_t num);

char *strerror(int errnum);

#endif // __posix_string_h
