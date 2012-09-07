#ifndef EXOS_TIME_HAL_H
#define EXOS_TIME_HAL_H

// prototypes
void hal_time_initialize(int period_us);

// defined in kernel
void __kernel_tick();

#endif // EXOS_TIME_HAL_H
