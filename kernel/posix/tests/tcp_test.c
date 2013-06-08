#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <kernel/memory.h>

static void *_service(void *args);

void main()
{
	int err;
	
	int server = socket(AF_INET, SOCK_STREAM, 0);
	if (server >= 0)
	{
		struct sockaddr_in local = (struct sockaddr_in) {
			.sin_family = AF_INET,
			.sin_addr.s_addr = INADDR_ANY,
			.sin_port = htons(23) };

		// TODO: set socket options, buffer size (currently hacked, = 32)
		if (-1 != bind(server, (struct sockaddr *)&local, sizeof(local)) &&
			-1 != listen(server, 1))
		{
			while(1)
			{
				struct sockaddr_in remote;
				socklen_t remote_len;
				int fd = accept(server, (struct sockaddr *)&remote, &remote_len);
				if (fd == -1) break;

				pthread_t child;
				err = pthread_create(&child, NULL, _service, &fd);
				if (err == -1)
				{
					close(fd);
				}
			}
		}

		err = errno;
		close(server);
	}
}


static void *_service(void *args)
{
	int fd = *(int *)args;
	unsigned char buffer[256];

	pthread_t thread = pthread_self();
	int done = sprintf(buffer, "thread 0x%lx, socket %lx\r\n", thread.info, fd);
	write(fd, buffer, done);

	done = sprintf(buffer, "0x%lx bytes free in heap\r\n", exos_mem_heap_avail());
	write(fd, buffer, done);

	while(1)
	{
		done = read(fd, buffer, 256);
		if (done == -1)
			break;
		
		write(fd, buffer, done);
	}
	
	close(fd);

	return NULL;
}




