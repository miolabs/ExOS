#include <pthread.h>
#include <unistd.h>
#include <support/board_hal.h>

int pthread_tests()
{
	while(1)
	{
		hal_led_set(0, 1); // led on

		sleep(1);

		hal_led_set(0, 0); // led off

		sleep(1);
	}
}
