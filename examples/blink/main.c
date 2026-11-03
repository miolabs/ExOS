// blink -- the "hello world" of the ExOS virtual board.
//
// Two POSIX threads each blink an LED a few times under a shared mutex. On real
// hardware board_led_set() would toggle a GPIO; on the virtual board it is a
// no-op, so we also print the state to the console (/dev/console -> stdout).
//
// Build & run (from the repo root):
//   cmake -S . -B build && cmake --build build && ./build/examples/blink

#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <board.h>

static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;

static void _sleep_ms(long ms)
{
	struct timespec ts = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000 };
	nanosleep(&ts, &ts);
}

static void *_blink(void *arg)
{
	board_led_t led = (board_led_t)(long)arg;

	for (int i = 0; i < 3; i++)
	{
		pthread_mutex_lock(&_mutex);

		board_led_set(led, true);
		printf("led %d on\n", (int)led);
		_sleep_ms(200);

		board_led_set(led, false);
		printf("led %d off\n", (int)led);
		_sleep_ms(200);

		pthread_mutex_unlock(&_mutex);
	}
	return NULL;
}

int main(void)
{
	printf("blink: starting\n");

	pthread_t t1, t2;
	pthread_create(&t1, NULL, _blink, (void *)(long)LED_STATUS);
	pthread_create(&t2, NULL, _blink, (void *)(long)LED_USB);

	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	printf("blink: done\n");
	return 0;
}
