#include <kernel/thread.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <support/board_hal.h>

#define __aligned __attribute__((aligned(8)))

#define TEST_THREAD_STACK 256
static unsigned char _thread1_stack[TEST_THREAD_STACK] __aligned;
static unsigned char _thread2_stack[TEST_THREAD_STACK] __aligned;
static EXOS_THREAD _thread1, _thread2;

static volatile int _shared_int;

static void _set_shared_int(void *arg)
{
	_shared_int = *(int *)arg;
}

static int _basic_thread_test()
{
	exos_thread_set_pri(0);
	_shared_int = 0xbeef;
	int local_int = 0xcafe;

	exos_thread_sleep(10);	// idle should run now

	exos_thread_create(&_thread1, 1, _thread1_stack, TEST_THREAD_STACK, _set_shared_int, &local_int);
	// we should be suspended now until thread1 finishes

	if (_thread1.State != EXOS_THREAD_FINISHED) return -1;
	if (_shared_int != 0xcafe) return -2;

	local_int = 0xbeef;
	exos_thread_create(&_thread1, -1, _thread1_stack, TEST_THREAD_STACK, _set_shared_int, &local_int);
	// thread1 should be waiting now for us to sleep

	if (_thread1.State != EXOS_THREAD_READY) return -3;
	if (_shared_int != 0xcafe) return -4;

	exos_thread_sleep(10);	// thread1 should run now

	if (_thread1.State != EXOS_THREAD_FINISHED) return -5;
	if (_shared_int != 0xbeef) return -6;

	return 0;
}

static EXOS_EVENT _event;

static void _event_counter(void *arg)
{
//	int *pvalue = (int *)arg;
//	while(!_event.State)
//	{
//		(*pvalue)++;
//		exos_event_wait(&_event, EXOS_TIMEOUT_NEVER);
//	}
}

static int _event_test()
{
	exos_thread_set_pri(0);
	exos_thread_sleep(10);	// idle should run now

//	exos_event_create(&_event);
//	volatile int count1 = 0, count2= 0;
//	int done = exos_event_wait(&_event, 1000);
//	// should timeout
//
//	exos_thread_create(&_thread1, 1, _thread1_stack, TEST_THREAD_STACK, 
//		_event_counter, (void *)&count1);
//	if (count1 != 1 ||
//		_thread1.State != EXOS_THREAD_WAIT) return -1;
//	
//	exos_thread_create(&_thread2, 2, _thread2_stack, TEST_THREAD_STACK, 
//		_event_counter, (void *)&count2);
//	if (count2 != 1 ||
//		_thread1.State != EXOS_THREAD_WAIT) return -2;

	int iterations = 10000;
//	for (int i = 0; i < iterations; i++)
//	{
//		exos_event_reset(&_event);
//		if (count2 != count1) return -3;
//	}
//	if (count1 != (iterations + 1) ||
//		count2 != (iterations + 1))	return -4;
//	// TODO: check performance!
//
//	exos_event_set(&_event);
//	// both counter should have exit now
//	if (_thread1.State != EXOS_THREAD_FINISHED ||
//		_thread1.State != EXOS_THREAD_FINISHED) return -5;
//
//	done = exos_event_wait(&_event, 1000);
//	if (done != 0) return -6;
//	// should return ok, immediately

	return iterations;
}

static EXOS_MUTEX _mutex;

static void _mutex_func(void *arg)
{
	int led = (int)arg;
	
	for(int i = 0; i < 5; i++)
	{
		exos_mutex_lock(&_mutex);
		hal_led_set(led, 1);
		exos_thread_sleep(500);
		hal_led_set(led, 0);
		exos_mutex_unlock(&_mutex);
	}
	exos_event_reset(&_event);
}

static int _monitor_test()
{
	exos_thread_set_pri(0);
	exos_thread_sleep(10);	// idle should run now

	exos_event_create(&_event);	// ending event
	exos_mutex_create(&_mutex);

	exos_thread_create(&_thread1, -1, _thread1_stack, TEST_THREAD_STACK, 
		_mutex_func, (void *)1);
	exos_thread_create(&_thread2, -1, _thread2_stack, TEST_THREAD_STACK, 
		_mutex_func, (void *)2);

	exos_event_wait(&_event, EXOS_TIMEOUT_NEVER);
	exos_event_wait(&_event, EXOS_TIMEOUT_NEVER);
	return 0;
}

int threading_tests()
{
	if (_basic_thread_test() < 0)
		return -1;

	if (_event_test() < 0)
		return -2;

	if (_monitor_test() < 0)
		return -3;

	return 0;
}
