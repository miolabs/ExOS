#ifndef HAL_PWM_HAL_H
#define HAL_PWM_HAL_H

int hal_pwm_initialize(int module, int range, int rate);
void hal_pwm_set_output(int module, int channel, int value);
void hal_pwm_set_period(int module, int value);


#endif // HAL_PWM_HAL_H
