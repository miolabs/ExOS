// Your ExOS application.
//
// On the host "virtual board" the kernel boots first and then runs this main()
// as its initial thread -- so you write an ordinary Unix-style program. The
// same source builds for real hardware later, where main() is likewise the
// kernel's first thread.
//
// printf() goes to /dev/console, which the virtual board wires to stdout.

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

static void *_worker(void *arg)
{
	const char *name = (const char *)arg;
	for (int i = 1; i <= 3; i++)
	{
		printf("%s: tick %d\n", name, i);
		struct timespec ts = { .tv_nsec = 200000000 };		// 200 ms
		nanosleep(&ts, &ts);
	}
	return NULL;
}

int main(void)
{
	printf("hello from your ExOS app!\n");

	pthread_t t;
	pthread_create(&t, NULL, _worker, "worker");
	pthread_join(t, NULL);

	printf("done\n");
	return 0;
}
