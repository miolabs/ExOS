#ifndef EXOS_TIME_HAL_H
#define EXOS_TIME_HAL_H

#include <kernel/systime.h>

// prototypes
void hal_time_initialize(int period_us);
void hal_time_get_datetime(KDATETIME *datetime);
void hal_time_set_datetime(KDATETIME *datetime);

// defined in kernel
void __kernel_tick();

#endif // EXOS_TIME_HAL_H
