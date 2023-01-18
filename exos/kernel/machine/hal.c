#include "hal.h"
#include <kernel/panic.h>

#pragma GCC optimize(1)

__weak void __mem_copy(void *start, void *stop, const void *source)
 {
	if (source != start)
	{
		while(start != stop) *(unsigned char *)start++ = *(unsigned char *)source++;
	}
 }

__weak void __mem_set(void *start, void *stop, unsigned char stuff)
{
	while(start != stop) *(unsigned char *)start++ = stuff;
}

__weak void *memset(void *s, int c, unsigned long n)
{
	__mem_set(s, s + n, c);
	return s;
}

__weak void *memcpy(void *restrict s1, const void *restrict s2, unsigned long n)
{
	__mem_copy(s1, s1 + n, s2);
	return s1;
}

__weak unsigned int __str_copy(char *dst, const char *src, unsigned int max_length)
{
	unsigned int done = 0;
	while(done < max_length)
	{
		char c = src[done];
		if (c == '\0') break;
		dst[done++] = c;
	}

	if (done < max_length) dst[done] = '\0';
	return done;  
}

__weak int __str_comp(const char *str1, const char *str2)
{
	if (str1 == NULL || str2 == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
	if (str1 == str2) return 0;

	char a, b;
	do
	{
		a = *str1++;
		b = *str2++;
	} while (a == b && a != 0);

	if (a == b) return 0;
	return (a < b) ? -1 : 1;
}

static inline void _reverse_str(char* str, int len)
{
	int p = 0;
	int q = len - 1;
    while(p < q)
	{
        char c = str[p];
		str[p] = str[q];
		str[q] = c;
        ++p, --q;
	}
}

unsigned int __uint_hexlz(char *dst, unsigned int value, int tz, char tc)
{
	unsigned int length = 0;
	do
	{
		int digit = value & 0xF;
		value >>= 4;
		dst[length++] = (digit >= 10) ? digit + 'a' - 10 : digit + '0';
	} while(value != 0);

	while(length < tz)
		dst[length++] = tc; 

	_reverse_str(dst, length);
	return length;
}

unsigned int __uint_hexl(char *dst, unsigned int value)
{
	return __uint_hexlz(dst, value, 0, '\0');
}

unsigned int __int_declz(char *dst, int value, int tz, char tc) 
{
	unsigned int length = 0;
	if (value < 0)
	{
		*dst++ = '-';
		value = -value;
	}
	do
	{
		int digit = value % 10;
		value /= 10;
		dst[length++] = digit + '0';
	} while(value != 0);

	while(length < tz)
		dst[length++] = tc; 

	_reverse_str(dst, length);
	return length;
}

unsigned int __int_decl(char *dst, int value) 
{
	return __int_declz(dst, value, 0, '\0');
}

unsigned int __uint_declz(char *dst, unsigned int value, int tz, char tc) 
{
	unsigned int length = 0;
	do
	{
		unsigned int digit = value % 10U;
		value /= 10U;
		dst[length++] = digit + '0';
	} while(value != 0);

	while(length < tz)
		dst[length++] = tc; 

	_reverse_str(dst, length);
	return length;
}

unsigned int __uint_decl(char *dst, unsigned int value) 
{
	return __uint_declz(dst, value, 0, '\0');
}


#if __SIZEOF_INT__ == __SIZEOF_LONG__
#pragma weak __long_decl = __int_decl
#pragma weak __ulong_decl = __uint_decl
#pragma weak __ulong_hexl = __uint_hexl
#pragma weak __long_declz = __int_declz
#pragma weak __ulong_declz = __uint_declz
#pragma weak __ulong_hexlz = __uint_hexlz
#else
#error "Not implemented"
#endif

unsigned int __decl_uint(const char *src, unsigned int *pvalue)
{
	unsigned int done = 0;
	unsigned int value = 0;
	while(src[done] == ' ') done++;
	while(1)
	{
		char c = src[done];
		if (c >= '0' && c <= '9')
		{
			value = (value * 10) + (c - '0');
			done++;
		}
		else break;
	}
	*pvalue = value;
	return done;
}

unsigned int __decl_int(const char *src, int *pvalue)
{
	unsigned int done = 0;
	int value = 0;
	bool negative = false;
	while(src[done] == ' ') done++;
	if (src[done] == '-') 
	{
		done ++;
		negative = true;
	}
	while(1)
	{
		char c = src[done];
		if (c >= '0' && c <= '9')
		{
			value = (value * 10) + (c - '0');
			done++;
		}
		else break;
	}
	*pvalue = negative ? -value : value;
	return done;
}

