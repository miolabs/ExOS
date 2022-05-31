#ifndef __posix_signal_h
#define __posix_signal_h

#include <time.h>
#include <sys/types.h>

typedef unsigned char sig_atomic_t;
typedef unsigned long sigset_t;
 
int raise(int);


#endif // __posix_signal_h

