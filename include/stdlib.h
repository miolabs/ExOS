#ifndef __posix_stdlib_h
#define __posix_stdlib_h

#include <stddef.h>

typedef int div_t;
typedef long ldiv_t;
typedef long long lldiv_t; 

void          abort(void);
int           abs(int);
int           atexit(void (*)(void));
double        atof(const char *);
int           atoi(const char *);
long          atol(const char *);
long long     atoll(const char *);
void         *bsearch(const void *, const void *, size_t, size_t,
                  int (*)(const void *, const void *));
void         *calloc(size_t, size_t);
div_t         div(int, int);

void          free(void *);
void          exit(int);

char         *getenv(const char *);

long          labs(long);

ldiv_t        ldiv(long, long);
long long     llabs(long long);
lldiv_t       lldiv(long long, long long);

void         *malloc(size_t);
int           mblen(const char *, size_t);
size_t        mbstowcs(wchar_t *restrict, const char *restrict, size_t);
int           mbtowc(wchar_t *restrict, const char *restrict, size_t);

void          qsort(void *, size_t, size_t, int (*)(const void *,
                  const void *));
int           rand(void);

void         *realloc(void *, size_t);

void          srand(unsigned);

double        strtod(const char *restrict, char **restrict);
float         strtof(const char *restrict, char **restrict);
long          strtol(const char *restrict, char **restrict, int);
long double   strtold(const char *restrict, char **restrict);
long long     strtoll(const char *restrict, char **restrict, int);
unsigned long strtoul(const char *restrict, char **restrict, int);
unsigned long long
              strtoull(const char *restrict, char **restrict, int);
int           system(const char *);

size_t        wcstombs(char *restrict, const wchar_t *restrict, size_t);
int           wctomb(char *, wchar_t);

#endif // __posix_stdlib_h


