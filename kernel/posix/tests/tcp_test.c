#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

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
		err = bind(server, (struct sockaddr *)&local, sizeof(local));
		if (err != -1)
		{
			while (-1 != listen(server, 1))
			{
				struct sockaddr_in remote;
				socklen_t remote_len;
				int fd = accept(server, (struct sockaddr *)&remote, &remote_len);
				if (fd >= 0)
				{
					pthread_t *child = (pthread_t *)malloc(sizeof(pthread_t));
					err = pthread_create(child, NULL, _service, &fd);
					if (err == -1)
					{
						free(child);
						close(fd);
					}
				}
			}
		}
		close(server);
	}
}


static void *_service(void *args)
{
	int fd = *(int *)args;
	unsigned char buffer[256];

	while(1)
	{
		int done = read(fd, buffer, 256);
		if (done == -1)
			break;
		
		write(fd, buffer, done);
	}
	
	close(fd);
	return NULL;
}




