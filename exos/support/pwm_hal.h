#ifndef HAL_PWM_HAL_H
#define HAL_PWM_HAL_H

#include <kernel/types.h>
#include <stdbool.h>

typedef void (*pwm_handler_t)(unsigned module);

bool hal_pwm_initialize(unsigned module, unsigned range, unsigned rate, unsigned channel_count);
void hal_pwm_set_handler(unsigned module, unsigned channel, pwm_handler_t callback);
void hal_pwm_set_output(unsigned module, unsigned channel, unsigned value);
void hal_pwm_set_period(unsigned module, unsigned value);
void hal_pwm_sync(unsigned mask);

#endif // HAL_PWM_HAL_H
