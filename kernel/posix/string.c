#include <string.h>
#include <kernel/machine/hal.h>

char *strerror(int errnum)
{
	return "strerror() not implemented";
}

void *memset(void *s, int c, size_t n)
{
	__mem_set(s, s + n, c);
}

void *memcpy(void *restrict s1, const void *restrict s2, size_t n)
{
	__mem_copy(s1, s1 + n, s2);
}

int strcmp(const char *s1, const char *s2)
{
	return __str_comp(s1, s2);
}

size_t strlen(const char * str)
{
	int i=0;
	while(str[i])
		i++;
	return i;
}



