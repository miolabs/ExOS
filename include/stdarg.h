#ifndef __posix_stdarg_h
#define __posix_stdarg_h

typedef __builtin_va_list va_list;

#define va_start(v,l) \
    __builtin_va_start((v),l) 

#define va_arg \
    __builtin_va_arg

#define va_copy(d,s) \
    __builtin_va_copy((d),(s))

#define va_end(ap) \
    __builtin_va_end(ap)

#endif // __posix_stdarg_h
