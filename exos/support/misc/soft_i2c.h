#ifndef SOFT_I2C_H
#define SOFT_I2C_H

#include <stdbool.h>


// callbacks
void soft_i2c_assert_clk(bool data);
void soft_i2c_release_clk();
bool soft_i2c_read_data(bool *pdata);
bool soft_i2c_start(bool start);


#endif // SOFT_I2C_H


