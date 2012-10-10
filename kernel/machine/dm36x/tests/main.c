#include <kernel/thread.h>
#include <support/board_hal.h>

void main()
{
	while(1)
	{
		exos_thread_sleep(500);
		hal_led_set(0, 1);

		exos_thread_sleep(500);
		hal_led_set(0, 0);
	}
}