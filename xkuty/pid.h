#ifndef PID_H
#define PID_H

typedef struct
{
	float P, I, D;
	float CMin, CMax;
} PID_K;

typedef struct
{
	float Kp, Ti, Td;
} PID_KNORM;

typedef enum
{
	PID_KP = 0,
	PID_TI, PID_TD,
} PID_KNORM_E;

typedef struct
{
	float Integral;
	float Last;
	float SetPoint;
} PID_STATE;

// prototypes
float pid(PID_STATE *pid, float pv, const PID_K *k, float dt);
void pid_normalize_k(const PID_K *k, PID_KNORM *knorm);
void pid_setup_normalized(PID_K *k, PID_KNORM_E id, float value);

#endif // PID_H
