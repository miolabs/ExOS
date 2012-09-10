#include <kernel/thread.h>

#define __aligned __attribute__((aligned(8)))

#define TEST_THREAD_STACK 128
static unsigned char _thread1_stack[TEST_THREAD_STACK] __aligned;
static unsigned char _thread2_stack[TEST_THREAD_STACK] __aligned;
static EXOS_THREAD _thread1;

static int _shared_int;

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

int threading_tests()
{
	if (_basic_thread_test() != 0)
		return -1;



	return 0;
}
