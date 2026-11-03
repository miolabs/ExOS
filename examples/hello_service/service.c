// A minimal ExOS "service": background work wired in at kernel init.
//
// EXOS_INITIALIZER registers _service_start to run during kernel startup, at
// EXOS_INIT_SERVICE priority -- which is lower than EXOS_INIT_POSIX, so the
// console/stdout is already up by the time we print. The service spawns a
// worker thread using the native ExOS thread API; it runs concurrently with the
// application's main().
//
// This pattern is how drivers and long-running services register themselves
// without the application having to call them explicitly.

#include <kernel/thread.h>
#include <support/services/init.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>

static exos_thread_t _worker;
static unsigned char _worker_stack[1024];	// host ignores the size; bookkeeping only

static void *_service_thread(void *arg)
{
	(void)arg;
	for (int i = 1; i <= 3; i++)
	{
		printf("hello-service: tick %d\n", i);
		struct timespec ts = { .tv_nsec = 150000000 };	// 150 ms
		nanosleep(&ts, &ts);
	}
	printf("hello-service: stopping\n");
	return NULL;
}

static void _service_start(void)
{
	printf("hello-service: starting\n");
	exos_thread_create(&_worker, 0, _worker_stack, sizeof(_worker_stack),
		_service_thread, NULL);
}
EXOS_INITIALIZER(_hello_service, EXOS_INIT_SERVICE, _service_start);
