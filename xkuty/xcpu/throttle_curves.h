#ifndef XCPU_CURVES_H
#define XCPU_CURVES_H

#include "xkuty/xcpu.h"

// Return a fixed12 value
int get_curve_value(int fx12_in, XCPU_DRIVE_MODE mode);

void set_custom_curve_ptr(const unsigned char* curve);

#endif // XCPU_CURVES_H
