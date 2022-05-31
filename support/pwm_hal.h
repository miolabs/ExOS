#ifndef HAL_PWM_HAL_H
#define HAL_PWM_HAL_H

typedef void (*HAL_PWM_HANDLER)(int module);

int hal_pwm_initialize(int module, int range, int rate, int channel_for_period);
void hal_pwm_set_handler(int module, int channel, HAL_PWM_HANDLER callback);
void hal_pwm_set_output(int module, int channel, int value);
void hal_pwm_set_period(int module, int value);
void hal_pwm_sync(unsigned mask);

#endif // HAL_PWM_HAL_H
