// hello_service -- shows an application running alongside a background service.
//
// service.c registers a service at kernel init that spawns a worker thread.
// This main() is the application's initial thread; it just idles long enough to
// let the service's worker finish, then exits (which shuts the OS down).

#include <unistd.h>
#include <time.h>
#include <stdio.h>

int main(void)
{
	printf("app: started, letting the service run...\n");

	struct timespec ts = { .tv_sec = 1 };
	nanosleep(&ts, &ts);

	printf("app: exiting\n");
	return 0;
}
