#ifndef HAL_ADC_HAL_H
#define HAL_ADC_HAL_H

unsigned long hal_adc_initialize(int rate, int bits);
unsigned short hal_adc_read(int channel);
int hal_adc_read_no_wait(int channel, unsigned short *presult);

#endif // HAL_ADC_HAL_H
