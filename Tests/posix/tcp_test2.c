#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <kernel/memory.h>

static void *_service(void *args);

void main()
{
	int err;
	char buffer[256];
	
	int server = socket(AF_INET, SOCK_STREAM, 0);
	if (server >= 0)
	{
		struct sockaddr_in remote = (struct sockaddr_in) {
			.sin_family = AF_INET,
			.sin_addr.s_addr = inet_addr("169.254.24.248"),
			.sin_port = htons(800) };

		// TODO: set socket options, buffer size (currently hacked, = 32)
		if (-1 != connect(server, (struct sockaddr *)&remote, sizeof(remote)))
		{
			pthread_t thread = pthread_self();
			int done = sprintf(buffer, "thread 0x%lx, socket %lx\r\n", thread.info, socket);
			write(server, buffer, done);
		
			done = sprintf(buffer, "0x%lx bytes free in heap\r\n", exos_mem_heap_avail());
			write(server, buffer, done);
		
			while(1)
			{
				done = read(server, buffer, 256);
				if (done == -1)
					break;
				
				write(server, buffer, done);
			}
		}

		err = errno;
		close(server);
	}
}






