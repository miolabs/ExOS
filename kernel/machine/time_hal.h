#ifndef EXOS_TIME_HAL_H
#define EXOS_TIME_HAL_H

// prototypes
void hal_time_initialize(int period_us);

#ifdef EXOS_OLD_TICK_API
// defined in kernel
void __kernel_tick();
#endif

unsigned __machine_tick_elapsed();

#endif // EXOS_TIME_HAL_H
