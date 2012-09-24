#include <pthread.h>
#include <unistd.h>
#include <support/board_hal.h>

static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;

static void *_lock_func(void *arg)
{
	int led = (int)arg;

	for(int i = 0; i < 3; i++)
	{
		struct timespec blink = (struct timespec) { .tv_nsec = 500000 };
		pthread_mutex_lock(&_mutex);
		hal_led_set(led, 1); // led on
		nanosleep(&blink, &blink);
		hal_led_set(led, 0); // led off		
		pthread_mutex_unlock(&_mutex);
	}
	return (void *)(led * 1000 + 1);
}

int pthread_tests()
{
	hal_led_set(0, 0);
	hal_led_set(1, 0);

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

