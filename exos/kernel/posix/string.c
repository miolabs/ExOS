#include <string.h>
#include <kernel/machine/hal.h>

char *strerror(int errnum)
{
	return "strerror() not implemented";
}

void *memset(void *s, int c, size_t n)
{
	__mem_set(s, s + n, c);
	return s;
}

void *memcpy(void *restrict s1, const void *restrict s2, size_t n)
{
	__mem_copy(s1, s1 + n, s2);
	return s1;
}

int strcmp(const char *s1, const char *s2)
{
	return __str_comp(s1, s2);
}

size_t strlen(const char *str)
{
	int i=0;
	while(str[i])
		i++;
	return i;
}

char *strcpy(char *dest, const char *src)
{
	__str_copy(dest, src, -1);
	return dest;
}

int strncmp(const char *str1, const char *str2, size_t num)
{
	for (size_t index = 0; index < num; index++)
	{
		char c1 = str1[index];
		char c2 = str2[index];
		if (c1 == c2)
		{
			if (c1 == 0)
				break;
			continue;
		}
		else return c1 < c2 ? -1 : 1;
	}
	return 0;
}

char *strncpy(char *dest, const char *src, size_t num)
{
	unsigned length = 0;
	while(length < num)
	{
		char c = src[length];
		dest[length++] = c;
		if (c == '\0')
		{
			while(length < num)
				dest[length++] = '\0';
			break;
		}
	}
	return dest;
}

