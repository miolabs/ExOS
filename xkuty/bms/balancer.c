#include "balancer.h"
#include <support/misc/pca9635.h>

static int _balancing = 0;

int balancer_initialize()
{
	return pca9635_initialize(PCA9635_MODE_INVERT | PCA9635_MODE_TOTEMPOLE | PCA9635_MODE_OUTNE_2);
}

void balancer_on(int cell)
{
	balancer_off();

	_balancing = cell + 1;
	pca9635_set_output(_balancing, PCA9635_LED_ON);
	pca9635_set_output(0, PCA9635_LED_OFF);	// turn led on (inverted)
}

void balancer_off()
{
	pca9635_set_output(0, PCA9635_LED_ON);	// turn led off (inverted)
}


