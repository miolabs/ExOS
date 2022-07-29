#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include "../board/board.h"

static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;

static void *_lock_func(void *arg)
{
	int led = (int)arg;

	for(int i = 0; i < 3; i++)
	{
		pthread_mutex_lock(&_mutex);

		board_led_set(led, true); // led on
		printf("led %d on\n", led);

		struct timespec blink1 = (struct timespec) { .tv_nsec = 500000000 };
		nanosleep(&blink1, &blink1);

		board_led_set(led, false); // led off		
		printf("led %d off\n", led);

		struct timespec blink2 = (struct timespec) { .tv_nsec = 500000000 };
		nanosleep(&blink2, &blink2);

		pthread_mutex_unlock(&_mutex);
	}
	return (void *)(led * 1000 + 1);
}

int main()
{
	board_led_set(0, false);
	board_led_set(1, false);

	pthread_t t1, t2;
	int r1, r2;
	if (pthread_create(&t1, NULL, _lock_func, (void *)0) == 0)
	{
		if (pthread_create(&t2, NULL, _lock_func, (void *)1) == 0)
		{
			pthread_join(t2, (void **)&r2);
		}
		pthread_join(t1, (void **)&r1);
	}
	return 0;
}

