#ifndef HAL_ADC_HAL_H
#define HAL_ADC_HAL_H

#include <stdbool.h>

bool hal_adc_initialize(unsigned rate, unsigned bits);
bool hal_adc_read(unsigned channel, unsigned short *result);
bool hal_adc_start(unsigned ch_mask);
bool hal_adc_read_no_wait(unsigned channel, unsigned short *presult);

#endif // HAL_ADC_HAL_H
