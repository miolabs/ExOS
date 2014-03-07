#ifndef XCPU_SPEED_H
#define XCPU_SPEED_H

typedef struct 
{
	float speed;
	float dt;
	float ratio;
	float s_partial; 
} XCPU_SPEED_DATA;

// prototypes
void xcpu_speed_initialize();
//int xcpu_speed_read(float *pdt);
void xcpu_speed_calculation(XCPU_SPEED_DATA *sp, int mag_miles, float wheel_ratio_adjust);

#endif // XCPU_SPEED_H


