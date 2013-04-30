#include "pid.h"

#define PID_MIN_DIV 0.00001
#define PID_MAX_DIV 100000

float pid(PID_STATE *pid, float pv, const PID_K *k, float dt)
{
	if (pid)
	{
		float error = pid->SetPoint - pv;
		float control = error * k->P;
		if (k->I > PID_MIN_DIV || k->I < -PID_MIN_DIV)
		{
			pid->Integral += error * dt;
			float control_i = pid->Integral * k->I;
			
			// integral limit
			if (control_i > k->CMax) pid->Integral = k->CMax / k->I;
			else if (control_i < k->CMin) pid->Integral = k->CMin / k->I;

			control += control_i;
		}
		if (k->D > PID_MIN_DIV || k->D < -PID_MIN_DIV) control += (pv - pid->Last) * k->D / dt;
		pid->Last = pv;

		// control limit
		if (control > k->CMax) control = k->CMax;
		else if (control < k->CMin) control = k->CMin;
		return control;
	}
	else
	{
		return 0;
	}
}

void pid_normalize_k(const PID_K *k, PID_KNORM *knorm)
{
	*knorm = (PID_KNORM) { .Kp = k->P,
		.Ti = (k->I > PID_MIN_DIV || k->I < -PID_MIN_DIV) ?
			k->P / k->I : PID_MAX_DIV,
		.Td = (k->P > PID_MIN_DIV || k->P < -PID_MIN_DIV) ?
			k->D / k->P : PID_MAX_DIV };
}

void pid_setup_normalized(PID_K *k, PID_KNORM_E id, float value)
{
	PID_KNORM norm;
	switch(id)
	{
		case PID_KP:
			pid_normalize_k(k, &norm);
			k->P = value;
			k->I = (norm.Ti < PID_MAX_DIV && norm.Ti > -PID_MAX_DIV) ?
				k->P / norm.Ti : 0;
			k->D = k->P * norm.Td;
			break;
		case PID_TI:
			k->I = (value < PID_MAX_DIV && value > -PID_MAX_DIV) ?
				k->P / value : 0;
			break;
		case PID_TD:
			k->D = k->P * value;
			break;
	}
}

