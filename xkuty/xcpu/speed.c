#include <support/cap_hal.h>
#include "speed.h"

#define SPEED_TIMER_FREQ 10000
#define SPEED_BUFFER_SIZE 255
#define SPEED_MAX_PERIOD 50000

static void _handler(int channel, unsigned long time);
static unsigned short _buffer[SPEED_BUFFER_SIZE];
static volatile unsigned char _input_index, _output_index;

void speed_initialize()
{
	_input_index = _output_index = 0;
	hal_cap_initialize(CAP_TIMER_MODULE, SPEED_TIMER_FREQ, HAL_CAP_FALLING, _handler);
}

static unsigned long _time = 0;

static void _handler(int channel, unsigned long time)
{
	unsigned long elapsed = time - _time;
	_time = time;

	int index = _input_index + 1;
	if (index >= SPEED_BUFFER_SIZE) index = 0;
	if (index != _output_index)
	{
		if (elapsed <= SPEED_MAX_PERIOD)
			_buffer[index] = elapsed;
		else 
			_buffer[index] = 0;

		_input_index = index;
	}
}

int speed_read(float *pdt)
{
	int stop = 0;
	unsigned long elapsed = 0;
	int count = 0;
	while (_input_index != _output_index)
	{
		int index = _output_index;
		unsigned short time = _buffer[index];
		if (time == 0) 
		{
			stop = 1;
			time = SPEED_MAX_PERIOD;
		}
		count++;
		elapsed += time;
        
		index++;
		if (index >= SPEED_BUFFER_SIZE) index = 0;
		_output_index = index;
	}
	*pdt = (float)elapsed / SPEED_TIMER_FREQ;
	return stop ? 0 : count;
}




